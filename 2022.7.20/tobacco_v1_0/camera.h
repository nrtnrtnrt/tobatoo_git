/**
 * @file camera.h
 * @author DingKun & ChengLei
 * @date 2022.3.5
 * @brief The file contains a class named Camera
 * @details The file contains a class named Camera
 * which refers to the hyperspectral camera(FX10e).
 *
 * The Camera class in this file contains the operation
 * and parameters of the hyperspectral camera(FX10e).
 */

#ifndef CAMERA_H
#define CAMERA_H

#include <PvDevice.h>
#include <PvDeviceGEV.h>
#include <PvStream.h>
#include <PvStreamGEV.h>
#include <PvBuffer.h>
#include <PvSystem.h>
#include <QObject>
#include "iostream"
#include <iomanip>
#include <list>
#include "opencv.hpp"
#include <QThread>
#include <QSemaphore>
#include <QMutex>

using namespace std;

extern float file_explosure;
extern vector<int> mroi;
extern vector<int> valid;
extern int R;
extern int G;
extern int B;

class CaptureThread;
//Ebus图像传输流队列的大小
#define BUFFER_COUNT 64
//光谱数据的宽度
#define REALWIDTH 1024
//光谱数据的高度
#define REALHEIGHT 256
//有效的谱段
#define VALIDBANDS 22
//采集校正时取平均的图像数
#define CALIBRATION_FRAMES 35
//光谱相机的数字增益
#define GAIN 2.0f

#define RGB_HEIGHT 1024
#define RGB_WIDTH 4096

/**
 * @brief Indicate a pointer to function
 */
typedef int(*callback_ptr)(uint8_t*, uint64_t, uint64_t);

/**
 * @brief List contains pointer to PvBuffer
 */
typedef std::list<PvBuffer *> bufferlist;

/**
 * @brief Callback function runs when camera captures one frame
 * @param buffer The image data in one frame
 * @param frame_size The size of the buffer in one frame
 * @param frame_number The number of the frame
 * @return Indicate the status of operation
 */
int on_data_callback(uint8_t* buffer, uint64_t frame_size, uint64_t frame_number);

int onDataCallback_calibration(uint8_t* buffer, uint64_t frame_size, uint64_t frame_number);

int onDataCallback_acquisition(uint8_t* buffer, uint64_t frame_size, uint64_t frame_number);

/**
 * @brief The Camera class
 * @details
 * <pre>Hyperspectral camera model: Specim FX10e
 * etThe class contains the operation and paramers of the hyperspectral camera(FX10e).
 * Steps to control the hyperspectral camera:
 * 1. Selects a device
 * 2. Connect the PvDevice, opens the PvStream
 * 3. Allocates the buffers
 * 4. Starts acquistion
 * 5. Retrieve/process/free incoming buffers
 * 6. Stops acquisition</pre>
 */
class Camera : public QObject
{
    Q_OBJECT
public:
    PvDevice* lDevice = nullptr;       ///< Camera device handler
    PvStream* lStream = nullptr;       ///< Stream handler
    bufferlist lbufferlist;  ///< buffer list to manage memory

    PvGenCommand *lStart;    ///< GenICam command to start capture
    PvGenCommand *lStop;     ///< GenICam command to stop capture

    PvSystem lSystem;
    const PvDeviceInfo* lLastDeviceInfo = nullptr;    ///< DeviceInfo handler
    const PvDeviceInfo *lDeviceInfo = nullptr;

    PvGenParameterArray* parameter_array = nullptr;   ///< ParameterArray handler
    callback_ptr data_callback = nullptr;    ///< Pointer to function which type is callback_ptr

    CaptureThread* capture_thread = nullptr;

    bool capture_black_flag = false;   ///< flag to capture black frame
    bool capture_white_flag = false;   ///< flag to capture white frame

    cv::Mat sum_mat_for_calibration;
    cv::Mat eps;   ///<eps is setted to prevent division by 0

    cv::Mat white_mat;   ///<white frame for calibration
    cv::Mat black_mat;   ///<black frame for calibration

    float* white_buf = nullptr;
    float* black_buf = nullptr;

    double m_gain = GAIN;
    double m_exposuretime;
    double m_framerate;

    int64_t m_height;
    int single_frame_size;
    int multi_frame_size;

    explicit Camera(QObject *parent = nullptr);//默认构造函数

    ~Camera();

    /**
     * @brief Initialize the camera
     * @return Status
     * @retval  0 The camera is initialized successfully
     * @retval -1 Failed to enumerate and connect to the camera
     * @retval -2 Failed to configure the camera
     * @retval -3 Failed to create stream object and open stream
     * @retval -4 Failed to configure the stream
     * @details <pre>The function contains all the needed operations before the camera works.
     * Use function to initialize camera at first when you want to capture images. </pre>
     */
    int init_camera();

    /**
     * @brief Enumerate and connect to the camera
     * @return Status
     * @retval  0  The camera is connected successfully
     * @retval -1 lSystem.Find() error
     * @retval -2 lLastDeviceInfo is NULL, camera not found
     */
    int enum_connect_camera();

    /**
     * @brief Connect to the camera
     * @details Be sure to enumerate the camera first and lLastDeviceInfo is not NULL
     * @return Status
     * @retval  0 Connect to the camera successfully
     * @retval -1 PvDevice::CreateAndConnect() error
     * @retval -2 lLastDeviceInfo is NULL
     */
    int connect_camera();

    /**
     * @brief Get the parameters of camera
     */
    void get_camera_parameters();

    /**
     * @brief Set MROI
     * @details <pre>Setting MROI needs to happen as following sequence of commands:
     * 1. Disable MROI
     * 2. Clear old MROI indexes & values
     * 3. Set new MROI indexes & values
     * 4. Set STOP INDEX after the last known MROI index
     *    FX10/FX10e the STOP INDEX: MROI top offset = 1081, MROI height = 0
     * 5. Enable MROI</pre>
     * @param vec Vector contains mroi parameters
     */
    void set_MROI(const vector<int> &vec);

    /**
     * @brief Config the parameters of camera
     * @return Status
     * @retval  0 Configure camera parameters successfully
     * @retval -1 Failed to configure camera parameters
     */
    int config_camera();

    /**
     * @brief Open stream to the GigE Vision or USB3 Vision device
     * @return Status
     * @retval  0 The stream is opened successfully
     * @retval -1 lStream is NULL or PvStream::CreateAndOpen() error
     */
    int open_stream();

    /**
     * @brief Configure streaming for GigE Vision devices
     * @return Status
     * @retval  0 Configure stream successfully
     * @retval -1 lDeviceGEV is NULL
     * @details First, we use a dynamic cast to determine if the PvDevice object represents a GigE Vision device. If it is a
     * GigE Vision device, we do the required configuration. If it is a USB3 Vision device, no stream configuration is
     * required for this sample. When we create a pointer to the PvStream object, we use a static cast (because we already
     * know that the PvStream object represents a stream from a GigE Vision device (PvStreamGEV), and no checking is required)
     */
    int configure_stream();

    /**
     * @brief Create stream buffers to allocate memory for the received images
     * @details PvStream contains two buffer queues: an “input” queue and an “output” queue. First, we add PvBuffer
     * objects to the input queue of the PvStream object by calling PvStream::QueueBuffer once per buffer. As images
     * are received, PvStream populates the PvBuffers with images and moves them from the input queue to the output queue.
     * The populated PvBuffers are removed from the output queue by the application (using PvStream::RetrieveBuffer),
     * processed, and returned to the input queue (using PvStream::QueueBuffer)
     *
     * The memory allocated for PvBuffer objects is based on the resolution of the image and the bit depth of the
     * pixels (the payload) retrieved from the device using PvDevice::GetPayloadSize. The device returns the number of bytes
     * required to hold one buffer, based on the configuration of the device
     */
    void create_stream_buffers();

    /**
     * @brief Start acquisition
     * @author ChengLei
     * @details The function send start command to camera in order to start acquisition.
     * It will start a new thread CaptureThread to get image buffer from stream.
     */
    void start_acquisition();

    /**
     * @brief Stop acquisition
     * @author ChengLei
     * @details The function send stop command to camera in order to stop acquisition.
     */
    void stop_acquisition();

    /**
     * @brief Free stream buffers
     * @author Chenglei
     * @attention Remenber to free stream buffers before exiting the program
     */
    void free_stream_buffers();

    /**
     * @brief Register callback function
     * @param ptr Pointer to the callback function
     */
    inline void register_data_callback(callback_ptr ptr){
        data_callback = ptr;
    }

    /**
     * @brief Unregister callback function
     */
    void unregister_data_callback(){
        data_callback = nullptr;
    }

    /**
     * @brief Register callback function for calibration
     */
    void register_callback_calibration();

    /**
     * @brief Register callback function for acquisition
     */
    void register_callback_acquisition();

    int init_calibration_load();

signals:
    void send_calibration_finished_message();
    void send_data_to_ui(uint8_t*);

public slots:
    /**
     * @brief Load calibration file for black frame and white frame
     * @details read "./black" and "./white" files for black_buf and white_buf
     */
    void load_calibration();
};

/*--------------fx10 capturethread-------------------*/

/**
 * @brief The CaptureThread class
 * @details CaptureThread runs when camera starts capture.
 * Processing to captured buffers depends on callback function which registered before
 */
class CaptureThread: public QThread
{   Q_OBJECT

private:
    QMutex stop_mutex;

protected:
    /**
     * @brief CaptureThread runs when camera starts capture
     * @details Processing to captured buffers depends on callback function which registered before
     */
    void run();


public:
    bool m_stop = false;
    explicit CaptureThread(QObject *parent = NULL);
    void exit_thread();
};

/*-------------rgb_camera thread--------------*/
class RGB_CaptureThread: public QThread
{   Q_OBJECT

private:
    QMutex stop_mutex;

protected:
    /**
     * @brief CaptureThread runs when camera starts capture
     * @details Processing to captured buffers depends on callback function which registered before
     */
    void run();


public:
    bool m_stop = false;
    explicit RGB_CaptureThread(QObject *parent = NULL);
    void exit_thread();
};


/* -----------------------RGB_CAMERA--------------------- */
class RGB_Camera : public QObject
{
    Q_OBJECT
public:
    PvDevice* lDevice = nullptr;       ///< Camera device handler
    PvStream* lStream = nullptr;       ///< Stream handler
    bufferlist lbufferlist;  ///< buffer list to manage memory

    PvGenCommand *lStart;    ///< GenICam command to start capture
    PvGenCommand *lStop;     ///< GenICam command to stop capture
    PvGenCommand *lUserSetLoad;
    PvGenCommand *Blackcal;
    PvGenCommand *Whitecal;

    PvSystem lSystem;
    const PvDeviceInfo* lLastDeviceInfo = nullptr;    ///< DeviceInfo handler
    const PvDeviceInfo *lDeviceInfo = nullptr;

    PvGenParameterArray* parameter_array = nullptr;   ///< ParameterArray handler
    RGB_CaptureThread* rgb_capture_thread = nullptr;

//    double m_gain = 2;
//    double m_exposuretime;
//    double m_framerate;

    explicit RGB_Camera(QObject *parent = nullptr);

    ~RGB_Camera();

    int init_camera();
    int enum_connect_camera();
    int connect_camera();
    int config_camera();
    void get_camera_parameters();
    int open_stream();
    int configure_stream();
    void create_stream_buffers();
    void start_acquisition();
    void stop_acquisition();
    void free_stream_buffers();
};


#endif // CAMERA_H
