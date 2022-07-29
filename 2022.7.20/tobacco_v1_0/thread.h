/**
 * @file thread.h
 * @author DingKun & ChengLei
 * @date 2022.3.15
 * @brief The file contains three thread class SendThread,RecvThread,SaveThread and one class SaveBuf
 * @details SendThread sends image to python.RecvThread receives mask image from python.SaveThread save three
 * images by turns.SaveBuf maintenances the saved images.
 */

#ifndef THREAD_H
#define THREAD_H

#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMutex>
#include <QDateTime>
#include <QString>
#include "camera.h"

#define SAVEIMGNUMBER 2

/**
 * @brief The SendThread class
 * @details SendThread contains a send_server and a send_socket connect to python process.
 * It detects if send_buf is ready to send by semaphore.If send_buf is ready to send, it obtains valid
 * bands in send_buf then send to python process by send_socket
 */
class SendThread: public QThread
{   Q_OBJECT
protected:
    /**
     * @brief It detects if send_buf is ready to send by semaphore.If send_buf is ready to send, it obtains valid
     * bands in send_buf then send to python process by send_socket. Also, if the ACQUISITION_ENABLE flag is defined,
     * it is responsible for saving both calibrated image and raw image, but the mission of sending to python is stopped.
     */
    void run();

public:
    explicit SendThread(QObject *parent = NULL);
    ~SendThread();
    QTcpServer* send_server;
    QTcpSocket* send_socket;

    volatile bool save_flag = false;

signals:
    void send_valve_data(uint8_t*);
    void send_to_up_machine(float*);
    void send_mask(char* buf);
};


class SaveBuf
{
public:
    SaveBuf();
    ~SaveBuf();
    void enqueue(float* buf_tmp, char* mask_tmp);
    void save();

private:
    QMutex mutex;

    float* buf;
    char* mask;
    float* buf_copy;
    char* mask_copy;

    int index = 0;
};

class SaveThread: public QThread
{   Q_OBJECT
protected:
    void run();

public:
    explicit SaveThread(QObject *parent = NULL);
    QSemaphore save_flag;
};


#endif // THREAD_H
