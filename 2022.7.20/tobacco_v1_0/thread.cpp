/**
 * @file thread.cpp
 * @author DingKun & ChengLei
 * @date 2022.3.15
 * @brief The file contains the details about implementation of three thread class SendThread,RecvThread,SaveThread and one class SaveBuf
 * @details SendThread sends image to python.RecvThread receives mask image from python.SaveThread save three
 * images by turns.SaveBuf maintenances the saved images.
 */

#include "thread.h"
#include "unistd.h"
#include "fcntl.h"
#include "sys/types.h"
#include "sys/stat.h"


#define FIFO_PATH "/tmp/dkimg.fifo"
#define FIFO_MASK "/tmp/dkmask.fifo"
#define FIFO_RGB "/tmp/dkrgb.fifo"

#define ACQUISITION_ENABLE
#define PADDING 3
int padding = 0;

extern QSemaphore ready_to_send;
extern QSemaphore ready_to_send_rgb;
extern int file_threshold;
extern int file_threshold_rgb;

extern float* send_buf;
extern float* send_valid_buf;

extern uint8_t* send_rgb_buf;
//extern uint8_t* send_temp_rgb_buf;

uint8_t* recvbuf = nullptr;
extern uint8_t* temp_rgb;

extern Camera* camera;

SaveBuf* save_buf_queue = new SaveBuf();

uint8_t* merge_mask = new uint8_t[256 * REALHEIGHT];
uint8_t* merge_bit2byte = new uint8_t[32 * REALHEIGHT];

SendThread::SendThread(QObject *parent) : QThread(parent)
{
    recvbuf = new uint8_t[REALHEIGHT * REALWIDTH];
}

SendThread::~SendThread()
{
    if( send_server ) delete send_server;
    if( send_socket ) delete send_socket;
    if( recvbuf ) delete recvbuf;
}

void SendThread::run()
{
    //建立管道
    if(access(FIFO_PATH, F_OK) == -1)
    {
        int res = mkfifo(FIFO_PATH, 0777);
        if(res < 0)
        {
            cout << "make img fifo failed!" << endl;
            return;
        }
    }
    if(access(FIFO_MASK, F_OK) == -1)
    {
        int res = mkfifo(FIFO_MASK, 0777);
        if(res < 0)
        {
            cout << "make mask fifo failed!" << endl;
            return;
        }
    }
    if(access(FIFO_RGB, F_OK) == -1)
    {
        int res = mkfifo(FIFO_RGB, 0777);
        if(res < 0)
        {
            cout << "make rgb fifo failed!" << endl;
            return;
        }
    }

    int fd_img = open(FIFO_PATH, O_WRONLY);
    int fd_rgb = open(FIFO_RGB, O_WRONLY);

    string str = to_string(file_threshold);
    int ret = write(fd_img, str.c_str(), str.size());
    if(ret > 0)
        cout << "threshold spec size" << file_threshold << " send to python" << endl;

    str = to_string(file_threshold_rgb);
    ret = write(fd_rgb, str.c_str(), str.size());
    if(ret > 0)
        cout << "threshold rgb size" << file_threshold_rgb << " send to python" << endl;

    while(1)
    {
        ready_to_send.acquire();   //block if no send_buf is ready!
        ready_to_send_rgb.acquire();

        int k = 0;
        for( int i = 0; i < REALHEIGHT; i++ )
        {
            for(uint j = 0; j < valid.size(); j++ )
            {
                memcpy(send_valid_buf + REALWIDTH * k, send_buf + (i * camera->m_height + valid[j]) * 1024,
                       sizeof(float) * REALWIDTH);
                k++;
            }
        }
        /*存图功能*/
#ifdef ACQUISITION_ENABLE
        if(save_flag == true)
        {
            static int file_index = 0;
            string spec_filename = "./saved_img/spec" + to_string(file_index);
            FILE *fp = fopen(spec_filename.c_str(), "wb");
            fwrite(send_valid_buf, REALHEIGHT * REALWIDTH * valid.size() * 4, 1, fp);
            fclose(fp);
            string rgb_filename = "./saved_img/rgb" + to_string(file_index);
            fp = fopen(rgb_filename.c_str(), "wb");
            fwrite(send_rgb_buf, RGB_HEIGHT * RGB_WIDTH * 3, 1, fp);
            fclose(fp);
            file_index++;
            cout << "save img success!" << endl;
        }
        ///////////////////////////////
#endif

        //发送给python
        int ret = write(fd_img, send_valid_buf, REALWIDTH * valid.size() * REALHEIGHT * sizeof(float));

        ret = write(fd_rgb, send_rgb_buf, RGB_HEIGHT * RGB_WIDTH * 3);

        int fd = open(FIFO_MASK, O_RDONLY);
        read(fd, recvbuf, REALHEIGHT * REALWIDTH);
        close(fd);

        save_buf_queue->enqueue(send_valid_buf, (char*)recvbuf);

        //send mask to ui
        emit send_mask((char*)recvbuf);   //mask*********************************************

        /*mask图像转换喷阀通道  宽度1024像素对应256个喷阀，1024/256=4，每一行转换为256个bit，
         *总共32个字节，最终发送给下位机的数据大小为(32*256字节)*/

        int sum = 0;
        for(int i=0; i<REALWIDTH*REALHEIGHT; i+=4)
        {
            sum = recvbuf[i] + recvbuf[i+1] + recvbuf[i+2] + recvbuf[i+3];
            (sum > 0) ?  (merge_mask[i/4] = 1) :  (merge_mask[i/4] = 0);
            sum = 0;
        }
//        static int index_mask = 0;
//        string mask_name = "./mask_result" + to_string(index_mask);
//        FILE* fp = fopen(mask_name.c_str(), "wb");
//        fwrite(merge_mask, 256*256, 1, fp);
//        fclose(fp);

        //延长喷阀开启时间，纵向拉长像素
        uint8_t temp_buf[REALHEIGHT * REALWIDTH] = {0};
        for(int i=0; i<REALHEIGHT; ++i)
        {
            for(int j=0; j<REALWIDTH; ++j)
            {
                if(merge_mask[i * 256 + j] == 1)
                {
                    temp_buf[i * 256 + j] = 1;
                    padding = 0;
                    while(padding < PADDING)
                    {
                        if(i + padding >= REALHEIGHT)
                            break;
                        else
                        {
                            temp_buf[(i + padding) * 256 + j] = 1;
                            ++padding;
                        }
                    }
                }
            }
        }
//        string padding_name = "./padding_result" + to_string(index_mask);
//        fp = fopen(padding_name.c_str(), "wb");
//        fwrite(temp_buf, 256*256, 1, fp);
//        fclose(fp);
//        index_mask++;

        //计算喷伐开启次数
        /*
        for(int i=0; i<REALHEIGHT; ++i)
        {
            for(int j=0; j<256; ++j)
                valve_cnt[j] += merge_mask[i*REALHEIGHT+j];
        }
        */
        for(int i=0; i<256*REALHEIGHT; i+=8)
        {
            uint8_t temp = 0;
            for(int j=7; j>=0; --j)
                temp = (temp << 1) | temp_buf[i + j];
            merge_bit2byte[i/8] = temp;
        }
        emit send_valve_data(merge_bit2byte);
    }
}

SaveBuf::SaveBuf()
{
    buf = new float[REALWIDTH * VALIDBANDS * REALHEIGHT * SAVEIMGNUMBER];
    mask = new char[REALWIDTH * REALHEIGHT * SAVEIMGNUMBER];
    buf_copy = new float[REALWIDTH * VALIDBANDS * REALHEIGHT * SAVEIMGNUMBER];
    mask_copy = new char[REALWIDTH * REALHEIGHT * SAVEIMGNUMBER];
}

SaveBuf::~SaveBuf()
{
    delete [] buf;
    delete [] mask;
    delete [] buf_copy;
    delete [] mask_copy;
}

void SaveBuf::enqueue(float* buf_tmp, char* mask_tmp)
{
    mutex.lock();
    memcpy(buf + index * REALWIDTH * VALIDBANDS * REALHEIGHT, buf_tmp, REALWIDTH * VALIDBANDS * REALHEIGHT * 4);
    memcpy(mask + index * REALWIDTH * REALHEIGHT, mask_tmp, REALWIDTH * REALHEIGHT);
    mutex.unlock();
    index++;
//    cout << "save index ==================" << index << endl;
    if(index >= SAVEIMGNUMBER)
        index = 0;
}

void SaveBuf::save()
{
    mutex.lock();
    memcpy(buf_copy, buf, REALWIDTH * VALIDBANDS * REALHEIGHT * SAVEIMGNUMBER * 4);
    memcpy(mask_copy, mask, REALWIDTH * REALHEIGHT * SAVEIMGNUMBER);
    mutex.unlock();
    char* filename;
    QDateTime time = QDateTime::currentDateTime();
    QString str = time.toString("yyyyMMddhhmmss");
    for( int i = 0; i < SAVEIMGNUMBER; i++)
    {
        QString mask_name = str + "mask" + QString::number(i);
        filename = mask_name.toLatin1().data();
        FILE* fp = fopen(filename, "wb");
        fwrite(mask_copy + i * REALWIDTH * REALHEIGHT, REALWIDTH * REALHEIGHT, 1, fp);
        fclose(fp);

        QString buf_name = str + "buf" + QString::number(i) + ".raw";
        filename = buf_name.toLatin1().data();
        fp = fopen(filename, "wb");
        fwrite(buf_copy + i * REALWIDTH * VALIDBANDS * REALHEIGHT, REALWIDTH * VALIDBANDS * REALHEIGHT * 4, 1, fp);
        fclose(fp);
        cout << ">>> save success";
    }
}

//savethread保存图片线程，该线程保持一个环形队列始终存放实时图像
SaveThread::SaveThread(QObject *parent) : QThread(parent)
{}

void SaveThread::run()
{
//    cout << "save thread running" << endl;
    while (1)
    {
        save_flag.acquire();
        save_buf_queue->save();
        cout << "acquired!!" << endl;
    }
}

