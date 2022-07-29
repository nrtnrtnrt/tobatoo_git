#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "thread.h"
#include "camera.h"
#include "fstream"
#include "parameter.h"

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

    SendThread* send_thread = nullptr;
    SaveThread* save_thread = nullptr;

    cv::Mat img;
    QTcpServer* server_to_lowermachine = nullptr;
    QTcpSocket* lower_machine = nullptr;

    QTcpSocket* up_machine = nullptr;

    Parameter* parameter = nullptr;

    bool connect_to_monitor = false;
    bool send_to_up_machine = false;

    void set_spinbox_range();
    void set_ui(bool fullscreen);
    void connect_monitor();
    void load_parameter_doc();
    void load_system_parameter();
    void connect_signal();

    //communication protocal 通信协议
    void send_after_start();

    //记录相机开启关闭日志
    void time_log(int state);


private slots:
    void get_data(uint8_t* buff);

    void On_btn_start_clicked();

    void On_btn_stop_clicked();

    void On_btn_black_clicked();  //采集黑帧图片

    void On_btn_white_clicked();  //采集白帧图片

    void On_btn_capture_clicked();

    void get_mask(char* get_buf);

    void get_valve_data(uint8_t* valve_data);

    void get_send_to_up_machine(float* sendbuf);

    void readfrom_up_machine();
    
    void On_btn_sendsingle_clicked();

    void On_btn_autosend_clicked();

    void on_btn_1_clicked();

    void on_btn_2_clicked();

    void on_btn_3_clicked();

    void on_btn_4_clicked();

    void on_btn_5_clicked();

    void on_btn_6_clicked();

    void on_btn_7_clicked();

    void on_btn_8_clicked();

    void on_btn_9_clicked();

    void on_btn_0_clicked();

    void on_btn_clear_clicked();

    void on_btn_del_clicked();

    void on_btn_ensure_clicked();

    void on_btn_back_clicked();

    void on_btn_send_plus_clicked();

    void on_btn_send_min_clicked();

    void On_btn_threshold_clicked();

    void on_btn_set_lower_clicked();

    void on_btn_set_camera_clicked();

    void on_btn_set_clicked();

    void on_btn_rgb_white_clicked();

    void on_btn_rgb_black_clicked();

    void on_btn_save_enable_clicked();

    void on_btn_save_disenable_clicked();

private:
    Ui::Widget *ui;
};

#endif // WIDGET_H
