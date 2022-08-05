/**
 * @file camera.cpp
 * @author DingKun & ChengLei
 * @date 2022.3.8
 * @brief The file contains detailed implementation of class Camera
 * @details
 * <pre>The file contains detailed implementation of class Camera.
 * Such as initialize camera, enum camera, start capture, callback function, etc.
 * which refers to the hyperspectral camera(FX10e).</pre>
 */

#include "camera.h"
#include "unistd.h"

float file_explosure;
vector<int> mroi;
vector<int> valid;
int R;
int G;
int B;

Camera* camera;
RGB_Camera* rgb_camera;

float* send_buf = nullptr;
float* send_temp_buf = nullptr;
float* send_valid_buf = nullptr;

float* save_valid_buf = nullptr;

QSemaphore ready_to_send(0);     //光谱图像信号量
QSemaphore ready_to_send_rgb(0); //rgb图像信号量

uint8_t* send_rgb_buf = nullptr;
//uint8_t* send_temp_rgb_buf = nullptr;
//rgb

uint8_t* channel_b = nullptr;
uint8_t* channel_g = nullptr;
uint8_t* channel_r = nullptr;
uint8_t* temp_rgb = nullptr;

uint16_t* lines_buf = nullptr;

std::vector<cv::Mat> channel_rgb;

/*test for callback function, not used
int on_data_callback(uint8_t* buffer, uint64_t frame_size, uint64_t frame_number)
{
    cout << ">>> frame_number: " << frame_number << "  " << "frame_size: " << frame_size << endl;
    cv::Mat mat(1000, 3840, CV_8UC1, buffer);
    //cv::imwrite("./a.png", mat);
    return 0;
}
*/

Camera::Camera(QObject *parent) : QObject(parent)
{

}

Camera::~Camera()
{
    free_stream_buffers();//释放缓存流

    if( lStream != nullptr )
    {
        lStream->Close();
        PvStream::Free(lStream);//Free应该是父类的一个function
        cout << ">>> close and free lstream >>>" << endl;
    }

    if( lDevice != nullptr )
    {
        lDevice->Disconnect();
        PvDevice::Free(lDevice);
        cout << ">>> disconnect and free camera device >>>" << endl;
    }

    if( capture_thread != nullptr)
        delete capture_thread;

    delete [] send_buf;
    delete [] send_temp_buf;

    delete [] white_buf;
    delete [] black_buf;

    cout << "dkkkkkkkkkkkkkkk" << endl;
}

/**
 * @details <pre>Steps to initialize the hyperspectral camera:
 * 1. Enumerate and connect to the camera
 * 2. Configure the camera parameters
 * 3. Creates stream object
 * 4. Configure stream for GigE Vision devices
 * 5. Create stream buffers
 * 6. Map the GenICam AcquisitionStart and AcquisitionStop commands
 * 7. New object capture_thread
 * 8. Allocate memory for every buffers
 */
int Camera::init_camera()
{
    // 1.Enumerate the camera
    int status = enum_connect_camera();
    if(status != 0)
        return -1;

    // 2.Configure the camera parameters
    status = config_camera();
    if(status != 0)
        return -2;

    get_camera_parameters();


    // 3.Creates stream object
    status = open_stream();
    if(status != 0)
        return -3;

    // 4.Configure stream for GigE Vision devices
    status = configure_stream();
    if(status != 0)
        return -4;

    // 5.Create stream buffers
    create_stream_buffers();

    // Get device parameters need to control streaming
    PvGenParameterArray *lDeviceParams = lDevice->GetParameters();

    // 6.Map the GenICam AcquisitionStart and AcquisitionStop commands
    lStart = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStart" ) );
    lStop = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStop" ) );

    // 7.New object capture_thread
    capture_thread = new CaptureThread();

    //获取波段数
    PvGenInteger* lheight = parameter_array->GetInteger("Height");

    lheight->GetValue(m_height);

    single_frame_size = REALWIDTH * m_height;
    multi_frame_size = single_frame_size * REALHEIGHT;

    // 8.Allocate memory for every buffers
    white_buf = new float[single_frame_size];
    black_buf = new float[single_frame_size];

    send_buf = new float[multi_frame_size];
    send_temp_buf = new float[multi_frame_size];
    send_valid_buf = new float[REALWIDTH * valid.size() * REALHEIGHT];   //此处m_height要改成valid

    sum_mat_for_calibration = cv::Mat(m_height, REALWIDTH, CV_32F, cv::Scalar(0));
    eps = cv::Mat(m_height, REALWIDTH, CV_32F, cv::Scalar(0.00000001));   ///<eps is setted to prevent division by 0

    white_mat = cv::Mat(m_height, REALWIDTH, CV_32F, cv::Scalar(0));   ///<white frame for calibration
    black_mat = cv::Mat(m_height, REALWIDTH, CV_32F, cv::Scalar(0));   ///<black frame for calibration

    channel_r = new uint8_t[REALWIDTH * REALHEIGHT * 2];
    channel_g = new uint8_t[REALWIDTH * REALHEIGHT * 2];
    channel_b = new uint8_t[REALWIDTH * REALHEIGHT * 2];
    temp_rgb = new uint8_t[REALWIDTH * REALHEIGHT * 2 * 3];

    return 0;
}

int Camera::enum_connect_camera()
{
    PvResult lResult;
//    PvSystem lSystem;

    // Find all devices on the network.
    lResult = lSystem.Find();
    if ( !lResult.IsOK() )
    {
        cout << "*** PvSystem::Find Error: " << lResult.GetCodeString().GetAscii();
        return -1;
    }
    // Go through all interfaces接口
    uint32_t lInterfaceCount = lSystem.GetInterfaceCount();
    for ( uint32_t x = 0; x < lInterfaceCount; x++ )
    {
        if(lLastDeviceInfo != nullptr)
            break;
        // Get pointer to the interface.
//        lInterface = lSystem.GetInterface(x);

        // Go through all the devices attached to the interface
        uint32_t lDeviceCount = lSystem.GetInterface(x)->GetDeviceCount();
//        cout << "device count = " << lDeviceCount << endl;
        for ( uint32_t y = 0; y < lDeviceCount ; y++ )
        {
            lDeviceInfo = lSystem.GetInterface(x)->GetDeviceInfo( y );

//            cout << "  Device " << y << endl;
            cout << "    Display ID: " << lDeviceInfo->GetDisplayID().GetAscii() << endl;

            const PvDeviceInfoGEV* lDeviceInfoGEV = dynamic_cast<const PvDeviceInfoGEV*>( lDeviceInfo );

            if ( lDeviceInfoGEV != nullptr ) // Is it a GigE Vision device?
            {
//                cout << "    MAC Address: " << lDeviceInfoGEV->GetMACAddress().GetAscii() << endl;
//                cout << "    IP Address: " << lDeviceInfoGEV->GetIPAddress().GetAscii() << endl;
//                cout << "    Serial number: " << lDeviceInfoGEV->GetSerialNumber().GetAscii() << endl << endl;
                if(!strcmp("70:f8:e7:b0:07:c4", lDeviceInfoGEV->GetMACAddress().GetAscii()))
                {
                    PvDeviceGEV::SetIPConfiguration("70:f8:e7:b0:07:c4", "192.168.123.3", "255.255.255.0", "192.168.123.1");
                    lLastDeviceInfo = lDeviceInfo;
                    break;
                }
                else
                {
//                    cout << "not this!" << endl;
                    continue;
                }
            }    
        }
    }

    // Connect to the last device found
    if ( lLastDeviceInfo != nullptr )
    {
//        cout << ">>> lLastDeviceInfo is " << lLastDeviceInfo->GetDisplayID().GetAscii() << endl;
//        cout << ">>> Connecting to " << lLastDeviceInfo->GetDisplayID().GetAscii() << endl;

        // Creates and connects the device controller based on the selected device.
        lDevice = PvDevice::CreateAndConnect( lLastDeviceInfo, &lResult );
        if ( !lResult.IsOK() )
        {
            cout << "*** Unable to connect to " << lLastDeviceInfo->GetDisplayID().GetAscii() << endl;
            PvDevice::Free(lDevice);
            return -1;
        }
        else
        {
            cout << ">>> Successfully connected to " << lLastDeviceInfo->GetDisplayID().GetAscii() << endl;
            cout << endl;
            // Load parameters of the camera
            parameter_array = lDevice->GetParameters();

            return 0;
        }
    }
    else
    {
        cout << "*** No device found! ***" << endl;
        return -2;
    }
}

int Camera::open_stream()
{
    PvResult lResult;
    // Open stream to the GigE Vision or USB3 Vision device
//    cout << ">>> Opening stream to device." << endl;
    lStream = PvStream::CreateAndOpen( lLastDeviceInfo->GetConnectionID(), &lResult);
    if ( (lStream == nullptr) || !lResult.IsOK() )
    {
        cout << "*** Unable to stream from " << lLastDeviceInfo->GetDisplayID().GetAscii() << ". ***" << endl;
        PvStream::Free(lStream);
        PvDevice::Free(lDevice);
        return -1;
    }
//    else
//    {
//        cout << ">>> Opening stream successfully!" << endl;
//    }
    return 0;
}

int Camera::configure_stream()
{
    // If this is a GigE Vision device, configure GigE Vision specific streaming parameters
    PvDeviceGEV* lDeviceGEV = dynamic_cast<PvDeviceGEV *>( lDevice );
    if ( lDeviceGEV != nullptr )
    {
        PvStreamGEV *lStreamGEV = static_cast<PvStreamGEV *>( lStream );
        // Negotiate packet size
        lDeviceGEV->NegotiatePacketSize();
        // Configure device streaming destination
        lDeviceGEV->SetStreamDestination( lStreamGEV->GetLocalIPAddress(), lStreamGEV->GetLocalPort() );
//        cout << ">>> configure stream successfully!" << endl;
        return 0;
    }
    else
    {
        cout << "*** configure stream failed! ***" << endl;
        return -1;
    }
}

void Camera::create_stream_buffers()
{
    // Reading payload size from device
    uint32_t lSize = lDevice->GetPayloadSize();

    // Use BUFFER_COUNT or the maximum number of buffers, whichever is smaller
    uint32_t lBufferCount = ( lStream->GetQueuedBufferMaximum() < BUFFER_COUNT ) ? lStream->GetQueuedBufferMaximum() : BUFFER_COUNT;
    // Allocate buffers
    for( uint32_t i=0; i<lBufferCount; i++ )
    {
        // Create new buffer object
        PvBuffer *lBuffer = new PvBuffer;

        // Have the new buffer object allocate payload memory
        lBuffer->Alloc( static_cast<uint32_t>( lSize ) );

        // Add to external list - used to eventually release the buffers
        lbufferlist.push_back(lBuffer);
    }
//    cout << ">>> create stream buffers successfully" << endl;
}

void Camera::free_stream_buffers()//释放缓存流
{
    bufferlist::iterator iter = lbufferlist.begin();
    while( iter != lbufferlist.end() )
    {
        delete *iter;
        iter++;
    }
    // Clear the buffer list
    lbufferlist.clear();
}


void Camera::set_MROI(const vector<int> &vec)
{
    bool mroi_status = false;
    parameter_array->SetBooleanValue("MROI_Enable", mroi_status);

    int n = vec.size() / 2;

    parameter_array->SetIntegerValue("MROI_Index", n);
    parameter_array->SetIntegerValue("MROI_Y", 1081);
    parameter_array->SetIntegerValue("MROI_H", 0);

    //{100, 5, 300, 5}
    for( int i=0, j=0; i<n; i++,j+=2 )
    {
        parameter_array->SetIntegerValue("MROI_Index", i);
        parameter_array->SetIntegerValue("MROI_Y", vec[j]);
        parameter_array->SetIntegerValue("MROI_H", vec[j+1]);
    }


    mroi_status = true;
    parameter_array->SetBooleanValue("MROI_Enable", mroi_status);

}

int Camera::config_camera()
{
    PvResult lResult;
    //实际测试发现，快men有时无法自动打开，需要使用以下代码人为打开
    lResult = parameter_array->SetIntegerValue("MotorShutter_PulseFwd", 100);
    ::sleep(1);
    lResult = parameter_array->SetIntegerValue("MotorShutter_PulseRev", 100);
    ::sleep(1);
    lResult = parameter_array->SetIntegerValue("MotorShutter_PulseFwd", 200);
    ::sleep(1);
    lResult = parameter_array->SetIntegerValue("MotorShutter_PulseRev", 200);

    //设置触发模式为 外触发，线路Line0，上升沿触发
    lResult = parameter_array->SetEnumValue("TriggerMode", "On");
    lResult = parameter_array->SetEnumValue("TriggerSource", "Line0");


    //Set BinningVertical
    lResult = parameter_array->SetIntegerValue("BinningVertical", 1);
    if ( !lResult.IsOK() )
    {
        cout << "*** Unable to set <BinningVertical> *** " << endl;
        return -1;
    }

    //设置数据格式为12位
    lResult = parameter_array->SetEnumValue("PixelFormat", "Mono12");
    if ( !lResult.IsOK() )
    {
        cout << "*** Unable to set <PixelFormat> *** " << endl;
        return -1;
    }

    //Set gain
    lResult = parameter_array->SetFloatValue("Gain", m_gain);
    if ( !lResult.IsOK() )
    {
        cout << "*** Unable to set <Gain> *** " << endl;
        return -1;
    }

    //Set MROI
    set_MROI(mroi);

    //Set ExposureTime
    lResult = parameter_array->SetFloatValue("ExposureTime", file_explosure);
    if ( !lResult.IsOK() )
    {
        cout << "*** Unable to set <ExposureTime> *** " << endl;
        return -1;
    }
/* ---------------------设置为内触发-------------------------------*/
//    lResult = parameter_array->SetEnumValue("TriggerMode", "Off");
//    lResult = parameter_array->SetFloatValue("AcquisitionFrameRate", 1000.0f);

    return 0;
}


void Camera::get_camera_parameters()
{
    PvResult lResult;

    //Get BinningVertical
    PvGenInteger* lbinningvertical = parameter_array->GetInteger("BinningVertical");
    int64_t binning = 0;
    lResult = lbinningvertical->GetValue(binning);
    if( !lResult.IsOK() )
        cout << "*** Unable to get <BinningVertical> *** " << endl;
    else
        cout <<">>> BinningVertical: " << binning << endl;

    //Get Gain
    PvGenFloat* lgain = parameter_array->GetFloat("Gain");
    double gain = 0.0f;
    lResult = lgain->GetValue(gain);
    if( !lResult.IsOK() )
        cout << "*** Unable to get <Gain> *** " << endl;
    else
        cout <<">>> Gain: " << gain << endl;

    //Get Height
    PvGenInteger* lheight = parameter_array->GetInteger("Height");
    int64_t height = 0;
    lResult = lheight->GetValue(height);
    if( !lResult.IsOK() )
        cout << "*** Unable to get <Height> *** " << endl;
    else
        cout <<">>> Height: " << height << endl;

    //Get Width
    PvGenInteger* lwidth = parameter_array->GetInteger("Width");
    int64_t width = 0;
    lResult = lwidth->GetValue(width);
    if( !lResult.IsOK() )
        cout << "*** Unable to get <Width> *** " << endl;
    else
        cout <<">>> Width: " << width << endl;

    //Get AcquisitionFrameRate
    PvGenFloat* lframerate = parameter_array->GetFloat("AcquisitionFrameRate");
    double framerate = 0.0f;
    lResult = lframerate->GetValue(framerate);
    if( !lResult.IsOK() )
        cout << "*** Unable to get <AcquisitionFrameRate> *** " << endl;
    else
        cout <<">>> AcquisitionFrameRate: " << framerate << endl;

    //Get ExposureTime
    PvGenFloat* lexposuretime = parameter_array->GetFloat("ExposureTime");
    double exposuretime = 0.0f;
    lResult = lexposuretime->GetValue(exposuretime);
    if( !lResult.IsOK() )
        cout << "*** Unable to get <ExposureTime> *** " << endl;
    else
        cout << ">>> ExposureTime: " << exposuretime << endl;

    //查看触发模式
    PvString mode;
    PvGenEnum* ltriggermode = parameter_array->GetEnum("TriggerMode");
    ltriggermode->GetValue(mode);
    cout << ">>> TriggerMode: " << mode.GetAscii() << endl;
    ltriggermode = parameter_array->GetEnum("TriggerSource");
    ltriggermode->GetValue(mode);
    cout << ">>> TriggerSource: " << mode.GetAscii() << endl;
    cout << endl;
}

void Camera::start_acquisition()
{
    // Queue all buffers in the stream
    bufferlist::iterator iter = lbufferlist.begin();
    while( iter != lbufferlist.end() )
    {
        lStream->QueueBuffer(*iter);
        iter++;
    }
    cout << "queue buffer successfully..." << endl;

    // Enable streaming and send the AcquisitionStart command
//    cout << "Enabling streaming and sending AcquisitionStart command." << endl;
    lDevice->StreamEnable();
    lStart->Execute();

    // start a new thread CaptureThread
    capture_thread->m_stop = false;
    capture_thread->start();
}

void Camera::stop_acquisition()
{
    //线程停止！
    capture_thread->exit_thread();
    capture_thread->wait();
    if(capture_thread->isFinished())
    {
        cout << ">>> capture thread stopped successfully" << endl;
    }
}

// void Camera::register_data_callback(callback_ptr ptr)
// {
//     data_callback = ptr;
// }

// void Camera::unregister_data_callback()
// {
//     data_callback = nullptr;
// }

 void Camera::register_callback_calibration()
 {
     register_data_callback(onDataCallback_calibration);
 }

 void Camera::register_callback_acquisition()
 {
     register_data_callback(onDataCallback_acquisition);
 }

 int Camera::init_calibration_load()
 {
     FILE* fp = fopen("./white", "rb");
     fread(white_buf, 4, single_frame_size, fp);
     fclose(fp);
     fp = fopen("./black", "rb");
     fread(black_buf, 4, single_frame_size, fp);
     fclose(fp);
     white_mat = cv::Mat(m_height, REALWIDTH, CV_32F, white_buf);
     black_mat = cv::Mat(m_height, REALWIDTH, CV_32F, black_buf);

     cout << ">>> load calibration init success! >>> " << endl;
//     emit camera->send_calibration_finished_message();
     return 0;
 }

 void Camera::load_calibration()
 {
     cout << ">>> loading calibration file......" << endl;
     FILE* fp = fopen("./white", "rb");
     fread(white_buf, 4, single_frame_size, fp);
     fclose(fp);
     cout << ">>> white file loaded successfully! >>>" << endl;

     fp = fopen("./black", "rb");
     fread(black_buf, 4, single_frame_size, fp);
     fclose(fp);
     cout << ">>> black file loaded successfully! >>>" << endl;

     white_mat = cv::Mat(m_height, REALWIDTH, CV_32F, white_buf);
     black_mat = cv::Mat(m_height, REALWIDTH, CV_32F, black_buf);

     cout << ">>> load calibration finished! >>> " << endl;
     emit camera->send_calibration_finished_message();
 }


//definition of calibration callback function 校正回调函数
static uint16_t calibration_frame_count = 0;
int onDataCallback_calibration(uint8_t *buffer, uint64_t frame_size, uint64_t frame_number)
{
    //采集黑白帧
    cout << ">>> acquisition number: " << calibration_frame_count << endl;
    if( camera->capture_black_flag || camera->capture_white_flag )
    {
        if( calibration_frame_count != CALIBRATION_FRAMES )
        {
            cv::Mat temp = cv::Mat(camera->m_height, REALWIDTH, CV_16U, buffer);
            temp.convertTo(temp, CV_32F);
            camera->sum_mat_for_calibration += temp;
            calibration_frame_count++;
            return 0;
        }
        else
        {
            camera->stop_acquisition();
            if( camera->capture_black_flag )
            {
                camera->capture_black_flag = false;
                camera->black_mat = camera->sum_mat_for_calibration / CALIBRATION_FRAMES;
                FILE* fp = fopen("./black", "wb");
                fwrite(camera->black_mat.data, camera->m_height*REALWIDTH*4, 1, fp);
                fclose(fp);
                cout << ">>> black frame acquisition OK! >>>" << endl;
                camera->sum_mat_for_calibration = cv::Mat(camera->m_height, REALWIDTH, CV_32F, cv::Scalar(0));  //clear sum mat
            }
            else if( camera->capture_white_flag )
            {
                camera->capture_white_flag = false;
                camera->white_mat = camera->sum_mat_for_calibration / CALIBRATION_FRAMES;
                FILE* fp = fopen("./white", "wb");
                fwrite(camera->white_mat.data, camera->m_height*REALWIDTH*4, 1, fp);
                fclose(fp);
                cout << ">>> white frame acquisition OK! >>>" << endl;
                camera->sum_mat_for_calibration = cv::Mat(camera->m_height, REALWIDTH, CV_32F, cv::Scalar(0));  //clear sum mat
            }
            calibration_frame_count = 0;
            camera->unregister_data_callback();
            return 0;
        }
    }
    return 0;
}

//definition of acquisition callback function 采集回调函数
static uint16_t send_show_frame_count = 0;
/**
 * @brief Acquisition callback function
 * @param buffer
 * @param frame_size
 * @param frame_number
 * @return
 * @details splice 256 buffers into a whole image,
 */
int onDataCallback_acquisition(uint8_t *buffer, uint64_t frame_size, uint64_t frame_number)
{
//    if(frame_number % REALHEIGHT == 0)
//        cout << ">>> " << frame_number << endl;

    memcpy(channel_r + send_show_frame_count * REALWIDTH * 2, buffer+2048 * R, REALWIDTH * 2);
    memcpy(channel_g + send_show_frame_count * REALWIDTH * 2, buffer+2048 * G, REALWIDTH * 2);
    memcpy(channel_b + send_show_frame_count * REALWIDTH * 2, buffer+2048 * B, REALWIDTH * 2);

//    //校正后图片拼接
    cv::Mat img = cv::Mat(camera->m_height, REALWIDTH, CV_16U, buffer);
    img.convertTo(img, CV_32F);
//    //calibrate operation 校正操作
    cv::Mat calibrated_img = (img - camera->black_mat) / (camera->white_mat - camera->black_mat + camera->eps);
    memcpy( (send_temp_buf + send_show_frame_count * camera->single_frame_size), calibrated_img.data, camera->single_frame_size * 4);

//    memcpy( (raw_temp_buf + send_show_frame_count * camera->single_frame_size * 2), buffer, camera->single_frame_size * 2);

    send_show_frame_count++;

    if( send_show_frame_count == REALHEIGHT )
    {
        send_show_frame_count = 0;

        channel_rgb.emplace_back(cv::Mat(REALHEIGHT, REALWIDTH, CV_16UC1, channel_r));
        channel_rgb.emplace_back(cv::Mat(REALHEIGHT, REALWIDTH, CV_16UC1, channel_g));
        channel_rgb.emplace_back(cv::Mat(REALHEIGHT, REALWIDTH, CV_16UC1, channel_b));

        cv::Mat temp;
        cv::merge(channel_rgb, temp);
        memcpy(temp_rgb, temp.data, REALHEIGHT * REALWIDTH * 6);
        channel_rgb.clear();

        memcpy(send_buf, send_temp_buf, camera->multi_frame_size * 4);

        //release semaphore
        ready_to_send.release();

        //emit to ui
        camera->send_data_to_ui(temp_rgb);
    }
    return 0;
}

/*----------CaptureThread 采集线程----------*/
 CaptureThread::CaptureThread(QObject *parent) : QThread(parent), m_stop(false)
 {}

 void CaptureThread::run()
 {
     /*----将线程绑定至8号cpu，提高线程效率----*/
     cpu_set_t get;
     cpu_set_t mask;
     int num = sysconf(_SC_NPROCESSORS_CONF);
//     pthread_t thread_id = pthread_self();
//     cout << thread_id << endl;
     CPU_ZERO(&mask);
     CPU_SET(7, &mask);
     sched_setaffinity(0, sizeof(mask), &mask);
     CPU_ZERO(&get);
     sched_getaffinity(0, sizeof(get), &get);
     for(int i=0; i<num; ++i)
     {
         if(CPU_ISSET(i, &get))
            cout << "running on " << i << endl;
     }
     /*-----------------------------------*/
     while(1)
     {
         stop_mutex.lock();
         if(m_stop)
         {
             stop_mutex.unlock();
             send_show_frame_count = 0;

             // Tell the device to stop sending images.
             camera->lStop->Execute();

             // Disable streaming on the device
             camera->lDevice->StreamDisable();

             // Abort all buffers from the stream and dequeue
             camera->lStream->AbortQueuedBuffers();

             // Retrieve the buffers left in stream
             while ( camera->lStream->GetQueuedBufferCount() > 0 )
             {
                 PvBuffer *lBuffer = nullptr;
                 PvResult lOperationResult;
                 camera->lStream->RetrieveBuffer( &lBuffer, &lOperationResult );
             }
             return;
         }
         stop_mutex.unlock();
         PvBuffer *lBuffer = nullptr;
         PvResult lOperationResult;

         // Retrieve next buffer
         PvResult lResult = camera->lStream->RetrieveBuffer( &lBuffer, &lOperationResult, 1000 );
         if ( lResult.IsOK() )
         {
             if ( lOperationResult.IsOK() )
             {
                 // We now have a valid buffer. This is where you would typically process the buffer.
                 // Get image data
//                 uint64_t frame_size = camera->lDevice->GetPayloadSize();
                 uint8_t* image = lBuffer->GetDataPointer();

                 //callback function
                 camera->data_callback(image, 0, lBuffer->GetBlockID());
             }
             else
                 cout << "spec error: "<< lOperationResult.GetCodeString().GetAscii() << endl; // Non OK operational result

             // Re-queue the buffer in the stream object
             camera->lStream->QueueBuffer( lBuffer );
         }
         else
             cout << "*** Retrieve buffer failure, error: " << lResult.GetCodeString().GetAscii() << "\r"; // Retrieve buffer failure
     }
 }

 void CaptureThread::exit_thread()
 {
     stop_mutex.lock();
     m_stop = true;
     stop_mutex.unlock();
 }

 /*-------------RGB_CAMERA---------------------*/
 RGB_Camera::RGB_Camera(QObject *parent) : QObject(parent)
 {
 }

 RGB_Camera::~RGB_Camera()
 {
     free_stream_buffers();

     if( lStream != nullptr )
     {
         lStream->Close();
         PvStream::Free(lStream);
         cout << ">>> rgb_camera: close and free lstream >>>" << endl;
     }

     if( lDevice != nullptr )
     {
         lDevice->Disconnect();
         PvDevice::Free(lDevice);
         cout << ">>> rgb_camera: disconnect and free camera device >>>" << endl;
     }
/*
     if( capture_thread != nullptr)
         delete capture_thread;
*/

     cout << "rgb_camera: dkkkkkkkkkkkkkkk" << endl;
 }

 int RGB_Camera::init_camera()
 {
     int status = enum_connect_camera();
     if(status != 0)
         return -1;

     status = config_camera();
     if(status != 0)
         return -2;

     get_camera_parameters();

     status = open_stream();
     if(status != 0)
         return -3;

     status = configure_stream();
     if(status != 0)
         return -4;

     create_stream_buffers();

     rgb_capture_thread = new RGB_CaptureThread();

     send_rgb_buf = new uint8_t[RGB_HEIGHT * RGB_WIDTH * 3];
//     send_temp_rgb_buf = new uint8_t[RGB_HEIGHT * RGB_WIDTH * 3];

     return 0;
 }

 int RGB_Camera::enum_connect_camera()
 {
     PvResult lResult;
     lResult = lSystem.Find();
     if ( !lResult.IsOK() )
     {
         cout << "*** PvSystem::Find Error: " << lResult.GetCodeString().GetAscii();
         return -1;
     }
     uint32_t lInterfaceCount = lSystem.GetInterfaceCount();
     for ( uint32_t x = 0; x < lInterfaceCount; x++ )
     {
         if(lLastDeviceInfo != nullptr)
             break;
         uint32_t lDeviceCount = lSystem.GetInterface(x)->GetDeviceCount();
         for ( uint32_t y = 0; y < lDeviceCount ; y++ )
         {
             lDeviceInfo = lSystem.GetInterface(x)->GetDeviceInfo( y );
//             cout << "    Display ID: " << lDeviceInfo->GetDisplayID().GetAscii() << endl;
             const PvDeviceInfoGEV* lDeviceInfoGEV = dynamic_cast<const PvDeviceInfoGEV*>( lDeviceInfo );
             if ( lDeviceInfoGEV != nullptr ) // Is it a GigE Vision device?
             {
//                 cout << "    MAC Address: " << lDeviceInfoGEV->GetMACAddress().GetAscii() << endl;
//                 cout << "    IP Address: " << lDeviceInfoGEV->GetIPAddress().GetAscii() << endl;
//                 cout << "    Serial number: " << lDeviceInfoGEV->GetSerialNumber().GetAscii() << endl << endl;
 //                lLastDeviceInfo = lDeviceInfo;
                 if(!strcmp("00:26:ac:a1:00:9d", lDeviceInfoGEV->GetMACAddress().GetAscii()))
                 {
                     PvDeviceGEV::SetIPConfiguration("00:26:ac:a1:00:9d", "192.168.15.105", "255.255.255.0", "192.168.15.1");
                     lLastDeviceInfo = lDeviceInfo;
                     break;
                 }
                 else
                 {
//                     cout << "rgb: not this!" << endl;
                     continue;
                 }
             }
         }
     }

     // Connect to the last device found
     if ( lLastDeviceInfo != nullptr )
     {
//         cout << ">>> lLastDeviceInfo is " << lLastDeviceInfo->GetDisplayID().GetAscii() << endl;
         cout << ">>> Connecting to " << lLastDeviceInfo->GetDisplayID().GetAscii() << endl;

         // Creates and connects the device controller based on the selected device.
         lDevice = PvDevice::CreateAndConnect( lLastDeviceInfo, &lResult );
         if ( !lResult.IsOK() )
         {
             cout << "*** Unable to connect to " << lLastDeviceInfo->GetDisplayID().GetAscii() << endl;
             PvDevice::Free(lDevice);
             return -1;
         }
         else
         {
             cout << ">>> Successfully connected to " << lLastDeviceInfo->GetDisplayID().GetAscii() << endl;
             cout << endl;
             // Load parameters of the camera
             parameter_array = lDevice->GetParameters();
             return 0;
         }
     }
     else
     {
         cout << "*** No device found! ***" << endl;
         return -2;
     }
 }

 void RGB_Camera::get_camera_parameters()
 {
     PvResult lResult;

     //Get Height
     PvGenInteger* lheight = parameter_array->GetInteger("SensorHeight");
     int64_t height = 0;
     lResult = lheight->GetValue(height);
     if( !lResult.IsOK() )
         cout << "*** Unable to get <Height> *** " << endl;
     else
         cout <<">>> Height: " << height << endl;

     //Get Width
     PvGenInteger* lwidth = parameter_array->GetInteger("SensorWidth");
     int64_t width = 0;
     lResult = lwidth->GetValue(width);
     if( !lResult.IsOK() )
         cout << "*** Unable to get <Width> *** " << endl;
     else
         cout <<">>> Width: " << width << endl;

     //Get AcquisitionFrameRate
     PvGenFloat* lframerate = parameter_array->GetFloat("AcquisitionLineRate");
     double framerate = 0.0f;
     lResult = lframerate->GetValue(framerate);
     if( !lResult.IsOK() )
         cout << "*** Unable to get <AcquisitionLineRate> *** " << endl;
     else
         cout <<">>> AcquisitionLineRate: " << framerate << endl;

     //Get ExposureTime
     PvGenFloat* lexposuretime = parameter_array->GetFloat("CommonExposureTime");
     double exposuretime = 0.0f;
     lResult = lexposuretime->GetValue(exposuretime);
     if( !lResult.IsOK() )
         cout << "*** Unable to get <ExposureTime> *** " << endl;
     else
         cout << ">>> ExposureTime: " << exposuretime << endl;

     //查看触发模式
     PvString mode;
     PvGenEnum* ltriggermode = parameter_array->GetEnum("LineTriggerMode");
     ltriggermode->GetValue(mode);
     cout << ">>> TriggerMode: " << mode.GetAscii() << endl;
     ltriggermode = parameter_array->GetEnum("LineTriggerSource");
     ltriggermode->GetValue(mode);
     cout << ">>> TriggerSource: " << mode.GetAscii() << endl;
     cout << endl;
 }

 int RGB_Camera::open_stream()
 {
     PvResult lResult;
     // Open stream to the GigE Vision or USB3 Vision device
     lStream = PvStream::CreateAndOpen( lLastDeviceInfo->GetConnectionID(), &lResult);
     if ( (lStream == nullptr) || !lResult.IsOK() )
     {
         cout << "*** Unable to stream from " << lLastDeviceInfo->GetDisplayID().GetAscii() << ". ***" << endl;
         PvStream::Free(lStream);
         PvDevice::Free(lDevice);
         return -1;
     }
     return 0;
 }

 int RGB_Camera::config_camera()
 {
     PvGenParameterArray *lDeviceParams = lDevice->GetParameters();
     lStart = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStart" ) );
     lStop = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStop" ) );
     lUserSetLoad = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "UserSetLoad" ) );
     Whitecal = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "FFCCalPRNU" ) );
     Blackcal = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "FFCCalFPN" ) );
     PvResult lResult = parameter_array->SetEnumValue("UserSetSelector", "UserSet3");
     if(!lResult.IsOK()){
         cout << "set userset3 failed" << endl;
         return -1;
     }
     lResult = lUserSetLoad->Execute();
     if(!lResult.IsOK()){
         cout << "userset load failed" << endl;
         return -1;
     }

     //    //设置触发模式为 外触发，线路Line0，上升沿触发
     lResult = parameter_array->SetEnumValue("LineTriggerMode", "On");
     if(!lResult.IsOK()){
         cout << "set triggermode failed" << endl;
         return -1;
     }

     lResult = parameter_array->SetEnumValue("LineTriggerSource", "Line2");
     if(!lResult.IsOK()){
         cout << "set triggersource failed" << endl;
         return -1;
     }


 //    lResult = parameter_array->SetIntegerValue("SensorHeight", 1024);
 //    if ( !lResult.IsOK() )
 //    {
 //        cout << "*** Unable to set <Height> *** " << endl;
 //        return -1;
 //    }

 /* ---------------------设置为内触发-------------------------------*/
 //    lResult = parameter_array->SetEnumValue("LineTriggerMode", "Off");
 //    lResult = parameter_array->SetFloatValue("AcquisitionLineRate", 4000.0f);

     return 0;
 }

 int RGB_Camera::configure_stream()
 {
     // If this is a GigE Vision device, configure GigE Vision specific streaming parameters
     PvDeviceGEV* lDeviceGEV = dynamic_cast<PvDeviceGEV *>( lDevice );
     if ( lDeviceGEV != nullptr )
     {
         PvStreamGEV *lStreamGEV = static_cast<PvStreamGEV *>( lStream );
         // Negotiate packet size
         PvResult lresult =  lDeviceGEV->NegotiatePacketSize();
         if( !lresult.IsOK() )
             cout << "negotiate packet size failed" << endl;
         // Configure device streaming destination
         lresult = lDeviceGEV->SetStreamDestination( lStreamGEV->GetLocalIPAddress(), lStreamGEV->GetLocalPort() );
         if( !lresult.IsOK() )
             cout << "set stream destination failed" << endl;
 //        cout << ">>> configure stream successfully!" << endl;
         return 0;
     }
     else
     {
         cout << "*** configure stream failed! ***" << endl;
         return -1;
     }
 }

 void RGB_Camera::create_stream_buffers()
 {
     // Reading payload size from device
     uint32_t lSize = lDevice->GetPayloadSize();
     cout << "payloadsize =  " << lSize << endl;
 //    cout << lStream->GetQueuedBufferMaximum() << endl;

     // Use BUFFER_COUNT or the maximum number of buffers, whichever is smaller
     uint32_t lBufferCount = ( lStream->GetQueuedBufferMaximum() < 16 ) ? lStream->GetQueuedBufferMaximum() : 16;
     cout << "lbuffercount = " << lBufferCount << endl;

     // Allocate buffers
     for( uint8_t i=0; i<lBufferCount; i++ )
     {
         // Create new buffer object
         PvBuffer *lBuffer = new PvBuffer;

         // Have the new buffer object allocate payload memory
         PvResult lresult = lBuffer->Alloc( static_cast<uint32_t>( lSize + 500 ) );
         if(!lresult.IsOK())
             cout << "dk failed" << endl;

         // Add to external list - used to eventually release the buffers
         lbufferlist.push_back(lBuffer);
     }
     cout << ">>> rgb: create stream buffers successfully" << endl;
 }

 void RGB_Camera::free_stream_buffers()
 {
     bufferlist::iterator iter = lbufferlist.begin();
     while( iter != lbufferlist.end() )
     {
         delete *iter;
         iter++;
     }
     // Clear the buffer list
     lbufferlist.clear();
 }

 void RGB_Camera::start_acquisition()
 {
     // Queue all buffers in the stream
     bufferlist::iterator iter = lbufferlist.begin();
     while( iter != lbufferlist.end() )
     {
         lStream->QueueBuffer(*iter);
         iter++;
     }
     cout << "queue buffer successfully..." << endl;

     // Enable streaming and send the AcquisitionStart command
 //    cout << "Enabling streaming and sending AcquisitionStart command." << endl;
     lDevice->StreamEnable();
     lStart->Execute();

     // start a new thread CaptureThread
     rgb_capture_thread->m_stop = false;
     rgb_capture_thread->start();
 }

 void RGB_Camera::stop_acquisition()
 {
     //线程停止！
     rgb_capture_thread->exit_thread();
     rgb_capture_thread->wait();
     if(rgb_capture_thread->isFinished())
     {
         cout << ">>>rgb capture thread stopped successfully" << endl;
     }
 }

 /*--------rgb_capture thread---------------------*/
 RGB_CaptureThread::RGB_CaptureThread(QObject *parent) : QThread(parent), m_stop(false)
 {}

 void RGB_CaptureThread::run()
 {
     /*----将线程绑定至6号cpu，提高线程效率----*/
//     cpu_set_t get;
//     cpu_set_t mask;
//     int num = sysconf(_SC_NPROCESSORS_CONF);
////     pthread_t thread_id = pthread_self();
////     cout << thread_id << endl;
//     CPU_ZERO(&mask);
//     CPU_SET(6, &mask);
//     sched_setaffinity(0, sizeof(mask), &mask);
//     CPU_ZERO(&get);
//     sched_getaffinity(0, sizeof(get), &get);
//     for(int i=0; i<num; ++i)
//     {
//         if(CPU_ISSET(i, &get))
//            cout << "rgb running on " << i << endl;
//     }
     /*-----------------------------------*/
     while(1)
     {
         stop_mutex.lock();
         if(m_stop)
         {
             stop_mutex.unlock();

             // Tell the device to stop sending images.
             rgb_camera->lStop->Execute();

             // Disable streaming on the device
             rgb_camera->lDevice->StreamDisable();

             // Abort all buffers from the stream and dequeue
             rgb_camera->lStream->AbortQueuedBuffers();

             // Retrieve the buffers left in stream
             while ( rgb_camera->lStream->GetQueuedBufferCount() > 0 )
             {
                 PvBuffer *lBuffer = nullptr;
                 PvResult lOperationResult;
                 rgb_camera->lStream->RetrieveBuffer( &lBuffer, &lOperationResult );
             }
             return;
         }
         stop_mutex.unlock();
         PvBuffer *lBuffer = nullptr;
         PvResult lOperationResult;

         // Retrieve next buffer
         PvResult lResult = rgb_camera->lStream->RetrieveBuffer( &lBuffer, &lOperationResult, 1000);
         if ( lResult.IsOK() )
         {
             if ( lOperationResult.IsOK() )
             {
                 // We now have a valid buffer. This is where you would typically process the buffer.
                 // Get image data
//                 uint64_t frame_size = camera->lDevice->GetPayloadSize();
                 uint8_t* image = lBuffer->GetDataPointer();
                 memcpy(send_rgb_buf, image, RGB_HEIGHT * RGB_WIDTH * 3);
                 /*save
                 string filename = "./rgbimg";
                 filename = filename + to_string(lBuffer->GetBlockID());
                 FILE* fp = fopen(filename.c_str(), "wb");
                 fwrite(send_rgb_buf, 4096*1024*3, 1, fp);
                 fclose(fp);
                 cout << "save rgb" << endl;
                */

                 //do something here

//                 cout << "rgb image ok ! " << endl;
                 ready_to_send_rgb.release();
             }
             else{
                cout << "rgb error: "<< lOperationResult.GetCodeString().GetAscii() << endl; // Non OK operational result
                cout << endl;
             }
             // Re-queue the buffer in the stream object
             rgb_camera->lStream->QueueBuffer( lBuffer );
         }
         else
             cout << "*** Retrieve buffer failure, error: " << lResult.GetCodeString().GetAscii() << "\r"; // Retrieve buffer failure
     }
 }

 void RGB_CaptureThread::exit_thread()
 {
     stop_mutex.lock();
     m_stop = true;
     stop_mutex.unlock();
 }
