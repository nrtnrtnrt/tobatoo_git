#include "widget.h"
#include "ui_widget.h"
#include "camera.h"
#include "QDebug"
#include <QLabel>
#include <QImage>
#include <QPixmap>
#include <QMessageBox>
#include "fcntl.h"
#include "unistd.h"

#define parameter_file "./parameter"
#define log_file "./log"
#define sys_file "./sys_parameter"
//#define FIFO_THRESHOLD "/tmp/dkthreshold.fifo"
#define THRESHOLD 5

extern Camera* camera;
extern RGB_Camera* rgb_camera;
int file_encoder;
int file_delay;
int file_valve;
int file_threshold;
int file_threshold_rgb;
string file_mroi;
string file_valid;

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    /*--- set spinbox ---*/
    set_spinbox_range();

    /*--- ui init ---*/
    set_ui(false);

    /*--- client socket to monitor ---*/
//    connect_monitor();

    /*--- server socket to guo ---*/
    server_to_lowermachine = new QTcpServer();
    server_to_lowermachine->listen(QHostAddress::Any, 13452);
    //----------------------------

    /*--- load parameter document ---*/
    load_parameter_doc();

    /*--- load system parameter ---*/
    parameter = new Parameter();
    load_system_parameter();

    /*--- camera init ---*/
    camera = new Camera();
    rgb_camera = new RGB_Camera();

    /* thread init */
    send_thread = new SendThread();
    save_thread = new SaveThread();

    cout << " welcome dk ! " << endl;
    int res = 0;
    res = camera->init_camera();
    while(res != 0)
    {
        cout << "*** init camera error, error code: " << res << " ***" << endl;
        cout << ">>> try to init camera again... >>>" << endl;
        res = camera->init_camera();
    }
    res = rgb_camera->init_camera();
    while(res != 0)
    {
        cout << "*** init rgb_camera error, error code: " << res << " ***" << endl;
        cout << ">>> try to init rgb_camera again... >>>" << endl;
        res = rgb_camera->init_camera();
    }

    if(camera->init_calibration_load() == 0)
        ui->lab_info->setText("校正已完成");

    time_log(1);

//监视器部分
//    if(connect_to_monitor)
//    {
//        up_machine->write("start");
//        cout << ">>> send state to monitor successfully! >>>" << endl;
//    }
    cout << ">>> init camera successfully " << endl;

    connect_signal();

    /* thread run */
    send_thread->start();
    save_thread->start();
    /* everything seems to be ok ?*/
    cout << " hello dk ! " << endl;
}

Widget::~Widget()
{
    time_log(0);
    if( camera != nullptr )
        delete camera;
    if( rgb_camera != nullptr )
        delete rgb_camera;
    if( server_to_lowermachine != nullptr )
        delete server_to_lowermachine;
    if( lower_machine != nullptr )
        delete lower_machine;
    if( up_machine != nullptr )
        delete up_machine;
    if( send_thread != nullptr )
        delete send_thread;
    if( save_thread != nullptr )
        delete save_thread;
    if( ui != nullptr)
        delete ui;
    if( parameter != nullptr )
        delete parameter;
}

void Widget::set_spinbox_range()
{
    ui->spinbox_explosure->setRange(0.0f, 2000.0f);
    ui->spinbox_explosure->setDecimals(2);
    ui->spinbox_encoder->setRange(0, 1000);
    ui->spinbox_delaytime->setRange(0, 3000);
    ui->spinbox_valve->setRange(0, 9999);
    ui->spinbox_R->setRange(0, 500);
    ui->spinbox_G->setRange(0, 500);
    ui->spinbox_B->setRange(0, 500);
    ui->spinBox_channel->setRange(1, 256);
    ui->spinBox_channel->setWrapping(true);
    ui->spinbox_threshold->setRange(1,16);
    ui->spinbox_threshold->setWrapping(true);
    ui->spinbox_threshold_rgb->setRange(1,16);
    ui->spinbox_threshold_rgb->setWrapping(true);
}

void Widget::set_ui(bool fullscreen)
{
    ui->tabWidget->findChildren<QTabBar*>().at(0)->hide();
    ui->tabWidget->setCurrentIndex(0);
    if(fullscreen)
        this->showFullScreen();
    ui->spinBox_channel->setValue(1);

    ui->btn_stop->setEnabled(false);
    ui->btn_save_disenable->setEnabled(false);
}

void Widget::connect_monitor()
{
    up_machine = new QTcpSocket();
    up_machine->connectToHost("172.20.10.4", 9001);
    cout << ">>> trying to connect monitor......... >>>" << endl;
    if(!up_machine->waitForConnected(2000))
    {
        cout << "*** failed connect to state monitor ***" << endl;
        connect_to_monitor = false;
    }
    else
    {
        connect_to_monitor = true;
        cout << ">>> connect to monitor successfully! >>> " << endl;
    }
}

void Widget::load_parameter_doc()
{
    ifstream fin;
    fin.open(parameter_file);
    string str;
    vector<string> config;
    while(getline(fin, str))
        config.push_back(str);

    /* 1.曝光时间
     * 2.分频系数
     * 3.延时时间
     * 4.喷阀分频系数
     * 5.MROI
     * 678.RGB
     * 9.有效谱段
     * 10.threshold_for_model
     * 11.threshold_rgb
    */
    file_explosure = atof(config[0].c_str());
    file_encoder = atoi(config[1].c_str());
    file_delay = atoi(config[2].c_str());
    file_valve = atoi(config[3].c_str());
    file_mroi = config[4];
    R = atoi(config[5].c_str());
    G = atoi(config[6].c_str());
    B = atoi(config[7].c_str());
    file_valid = config[8];
    file_threshold = atoi(config[9].c_str());
    file_threshold_rgb = atoi(config[10].c_str());

    ui->lineEdit_MROI->setText(QString::fromStdString(file_mroi));
    ui->lineEdit_VALID->setText(QString::fromStdString(file_valid));

    const char* s = file_mroi.c_str();
    char* p;

    p = strtok((char*)s, ",");
    do
    {
        mroi.push_back(atoi(p));
        p = strtok(NULL, ",");
    }while(p);

    s = file_valid.c_str();
    p = strtok((char*)s, ",");
    do
    {
        valid.push_back(atoi(p));
        p = strtok(NULL, ",");
    }while(p);

    ui->spinbox_explosure->setValue(file_explosure);
    ui->spinbox_delaytime->setValue(file_delay);
    ui->spinbox_encoder->setValue(file_encoder);
    ui->spinbox_valve->setValue(file_valve);
    ui->spinbox_R->setValue(R);
    ui->spinbox_G->setValue(G);
    ui->spinbox_B->setValue(B);
    ui->spinbox_threshold->setValue(file_threshold);
    ui->spinbox_threshold_rgb->setValue(file_threshold_rgb);
}

void Widget::load_system_parameter()
{
    ifstream fin;
    string str_tmp;
    /*sys_file存储形式
    1.光源时长
    2.密码
    */
    fin.open(sys_file);
    int index = 0;
    while(getline(fin, str_tmp)){
        parameter->file_info[index] = str_tmp;
        ++index;
    }
    fin.close();
    parameter->lamp_used_time = std::stol(parameter->file_info[0]);
    parameter->current_time = QDateTime::currentDateTime();

    cout << ">>> load system parameter successfully! >>> " << endl;
    /*测试数据读取
    cout << parameter->file_info[0] << endl;
    cout << parameter->file_info[1] << endl;
    cout << parameter->current_time.toString("yyyy MM.dd hh:mm:ss").toStdString() << endl;
    */
}

void Widget::connect_signal()
{
    connect(ui->btn_goto_sort, &QPushButton::clicked, this,
            [=]()
            {
                ui->tabWidget->setCurrentIndex(1);
            }
    );
    connect(ui->btn_3to0, &QPushButton::clicked, this,
            [=](){ui->tabWidget->setCurrentIndex(0);}
    );

    connect(ui->btn_start, SIGNAL(clicked()), this, SLOT(On_btn_start_clicked()));
    connect(ui->btn_stop, SIGNAL(clicked()), this, SLOT(On_btn_stop_clicked()));
    connect(ui->btn_1to0, &QPushButton::clicked, this,
            [=](){ui->tabWidget->setCurrentIndex(0);}
    );
    connect(ui->btn_0to3, &QPushButton::clicked, this,
            [=](){ui->tabWidget->setCurrentIndex(3);}
    );

    connect(ui->btn_black, SIGNAL(clicked()), this, SLOT(On_btn_black_clicked()));
    connect(ui->btn_white, SIGNAL(clicked()), this, SLOT(On_btn_white_clicked()));
    connect(ui->btn_1to2, &QPushButton::clicked, this,
            [=](){ui->tabWidget->setCurrentIndex(4);}
    );
    connect(ui->btn_2to1, &QPushButton::clicked, this,
            [=](){ui->tabWidget->setCurrentIndex(1);}
    );

    connect(ui->btn_exit, &QPushButton::clicked, this,
            [=]()
            {
                if(connect_to_monitor)
                {
                    up_machine->write("stop");
                    cout << ">>> send state to monitor successfully! >>>" << endl;
                }
                this->close();
            }
    );

    connect(ui->btn_load_calibration, SIGNAL(clicked()), camera, SLOT(load_calibration()));
    connect(ui->btn_capture, SIGNAL(clicked()), this, SLOT(On_btn_capture_clicked()));

    connect(camera, SIGNAL(send_data_to_ui(uint8_t*)), this, SLOT(get_data(uint8_t*)));
    connect(send_thread, SIGNAL(send_mask(char*)), this, SLOT(get_mask(char*)));

    connect(camera, &Camera::send_calibration_finished_message, this,
            [=]()
            {
                cout << "fsdfsd" << endl;
                QMessageBox::information(this, "提示", "load success!");
                ui->lab_info->setText("加载校正文件成功");
            }
    );

    connect(server_to_lowermachine, &QTcpServer::newConnection, this,
            [=]()
            {
                 cout << "new connection to lower machine!" << endl;
                 lower_machine = server_to_lowermachine->nextPendingConnection();
                 ui->lab_lowermachine_isconnect->setStyleSheet("QLabel{background-color:rgb(0, 255, 0);}");
            }
    );
    connect(send_thread, SIGNAL(send_valve_data(uint8_t*)), this, SLOT(get_valve_data(uint8_t*)));
    connect(send_thread, SIGNAL(send_to_up_machine(float*)), this, SLOT(get_send_to_up_machine(float*)));
//    connect(up_machine, SIGNAL(readyRead()), this, SLOT(readfrom_up_machine()));
    connect(ui->btn_send_single, SIGNAL(clicked()), this, SLOT(On_btn_sendsingle_clicked()));
    connect(ui->btn_autosend, SIGNAL(clicked()), this, SLOT(On_btn_autosend_clicked()));
    connect(ui->btn_threshold_ok, SIGNAL(clicked()), this, SLOT(On_btn_threshold_clicked()));
}

void Widget::send_after_start()
{
    QString delay_time = QString::fromStdString((to_string(file_delay)));
    if(delay_time.size() == 0)
        return;
    QString divide_parameter = QString::fromStdString((to_string(file_encoder)));
    if(divide_parameter.size() == 0)
        return;
    QString sv_parameter = QString::fromStdString((to_string(file_valve)));
    if(sv_parameter.size() == 0)
        return;
    int len_delay = delay_time.size();
    int len_divide = divide_parameter.size();
    int len_sv = sv_parameter.size();
    QByteArray delay_byte = delay_time.toLatin1();
    QByteArray divide_byte = divide_parameter.toLatin1();
    QByteArray sv_byte = sv_parameter.toLatin1();

    uint8_t* delay_buf = new uint8_t[len_delay + 8];
    delay_buf[0] = 0xAA;
    delay_buf[1] = 0x00;
    delay_buf[2] = len_delay + 2;
    delay_buf[3] = 's';
    delay_buf[4] = 'd';
    memcpy(delay_buf + 5, delay_byte.data(), len_delay);
    delay_buf[len_delay + 5] = 0xFF;
    delay_buf[len_delay + 6] = 0xFF;
    delay_buf[len_delay + 7] = 0xBB;
    if(lower_machine->isWritable())
        lower_machine->write((const char*)delay_buf, len_delay+8);
    delete [] delay_buf;

    uint8_t* divide_buf = new uint8_t[len_divide + 8];
    divide_buf[0] = 0xAA;
    divide_buf[1] = 0x00;
    divide_buf[2] = len_divide + 2;
    divide_buf[3] = 's';
    divide_buf[4] = 'c';
    memcpy(divide_buf + 5, divide_byte.data(), len_divide);
    divide_buf[len_divide + 5] = 0xFF;
    divide_buf[len_divide + 6] = 0xFF;
    divide_buf[len_divide + 7] = 0xBB;
    if(lower_machine->isWritable())
        lower_machine->write((const char*)divide_buf, len_divide+8);
    delete [] divide_buf;

    uint8_t* valve_divide_buf = new uint8_t[len_sv + 8];
    valve_divide_buf[0] = 0xAA;
    valve_divide_buf[1] = 0x00;
    valve_divide_buf[2] = len_sv + 2;
    valve_divide_buf[3] = 's';
    valve_divide_buf[4] = 'v';
    memcpy(valve_divide_buf + 5, sv_byte.data(), len_sv);
    valve_divide_buf[len_sv + 5] = 0xFF;
    valve_divide_buf[len_sv + 6] = 0xFF;
    valve_divide_buf[len_sv + 7] = 0xBB;
    if(lower_machine->isWritable())
        lower_machine->write((const char*)valve_divide_buf, 12);
    delete [] valve_divide_buf;

    uint8_t start_command[9] = {0};
    start_command[0] = 0xAA;
    start_command[1] = 0x00;
    start_command[2] = 0x03;
    start_command[3] = 's';
    start_command[4] = 't';
    start_command[5] = 0xFF;
    start_command[6] = 0xFF;
    start_command[7] = 0xFF;
    start_command[8] = 0xBB;
    if(lower_machine != nullptr && lower_machine->isWritable())
        lower_machine->write((const char*)start_command, 9);
}

void Widget::On_btn_start_clicked()
{
    //设置触发模式为 外触发，线路Line0，上升沿触发
    PvResult lResult;
    lResult = camera->parameter_array->SetEnumValue("TriggerMode", "On");
    if ( !lResult.IsOK() )
    {
        cout << "*** Set Internal TriggerMode Failed! *** " << endl;
        return;
    }
    lResult = camera->parameter_array->SetEnumValue("TriggerSource", "Line0");
    if ( !lResult.IsOK() )
    {
        cout << "*** Set Internal TriggerSource Failed! *** " << endl;
        return;
    }

    lResult = rgb_camera->parameter_array->SetEnumValue("LineTriggerMode", "On");
    if(!lResult.IsOK()){
        cout << "set triggermode failed" << endl;
        return;
    }

    lResult = rgb_camera->parameter_array->SetEnumValue("LineTriggerSource", "Line2");
    if(!lResult.IsOK()){
        cout << "set triggersource failed" << endl;
        return;
    }

    /*记录时长开始*/
    parameter->lamp_timer.start();

    camera->register_callback_acquisition();

    camera->start_acquisition();

    rgb_camera->start_acquisition();

    ui->lab_info->setText("Start...");
    cout << ">>> start acquisition >>>" << endl;
    ui->btn_1to2->setEnabled(false);
    //send to lower_machine
    if(lower_machine != nullptr && lower_machine->isWritable())
        send_after_start();
//    camera->get_camera_parameters();
    ui->btn_stop->setEnabled(true);
    ui->btn_start->setEnabled(false);
}

void Widget::On_btn_stop_clicked()
{
    camera->stop_acquisition();

    rgb_camera->stop_acquisition();

    /*停止计时器记录时长*/
    uint32_t time_elapsed = parameter->lamp_timer.elapsed() / 1000;
    parameter->lamp_used_time += time_elapsed;

    camera->unregister_data_callback();
    ui->lab_info->setText("Stop...");
    cout << ">>> stop acquisition >>>" << endl;
    ui->btn_1to2->setEnabled(true);

    uint8_t stop_command[9] = {0};
    stop_command[0] = 0xAA;
    stop_command[1] = 0x00;
    stop_command[2] = 0x03;
    stop_command[3] = 's';
    stop_command[4] = 'p';
    stop_command[5] = 0xFF;
    stop_command[6] = 0xFF;
    stop_command[7] = 0xFF;
    stop_command[8] = 0xBB;
    if(lower_machine != nullptr && lower_machine->isWritable())
    {
        lower_machine->write((const char*)stop_command, 9);
        lower_machine->flush();
    }

    parameter->file_info[0] = to_string(parameter->lamp_used_time);
    ofstream fout;
    fout.open(sys_file, ios::out | ios::trunc);
    for(auto s : parameter->file_info)
        fout << s << endl;
    fout.close();
    cout << "used time: " << parameter->file_info[0] << endl;
    ui->btn_start->setEnabled(true);
    ui->btn_stop->setEnabled(false);
}


void Widget::On_btn_black_clicked()
{
    //更改触发模式并设置行频
    PvResult lResult;
    lResult = camera->parameter_array->SetEnumValue("TriggerMode", "Off");
    if ( !lResult.IsOK() )
    {
        cout << "*** Set Internal TriggerMode Failed! *** " << endl;
        return;
    }
    lResult = camera->parameter_array->SetFloatValue("AcquisitionFrameRate", 100.0);
    if ( !lResult.IsOK() )
    {
        cout << "*** Unable to set <AcquisitionFrameRate> *** " << endl;
        return;
    }
    ui->lab_info->setText("等待校正...");

    camera->register_callback_calibration();
    camera->capture_black_flag = true;
    camera->start_acquisition();
}

void Widget::On_btn_white_clicked()
{
    //更改触发模式并设置行频
    PvResult lResult;
    lResult = camera->parameter_array->SetEnumValue("TriggerMode", "Off");
    if ( !lResult.IsOK() )
    {
        cout << "*** Set Internal TriggerMode Failed! *** " << endl;
        return;
    }
    lResult = camera->parameter_array->SetFloatValue("AcquisitionFrameRate", 100.0);
    if ( !lResult.IsOK() )
    {
        cout << "*** Unable to set <AcquisitionFrameRate> *** " << endl;
        return;
    }
    ui->lab_info->setText("等待校正...");

    camera->register_callback_calibration();
    camera->capture_white_flag = true;
    camera->start_acquisition();
}

void Widget::On_btn_capture_clicked()
{
    save_thread->save_flag.release(1);
}

void Widget::get_data(uint8_t *buff)
{
    img = cv::Mat(REALHEIGHT, REALWIDTH, CV_16UC3, buff);
    img.convertTo(img, CV_32FC3);
    img = img / 4095 * 255;
    img.convertTo(img, CV_8UC3);

//    const unsigned char *pSrc = (const unsigned char*)img.data;
//    QImage image(pSrc, img.cols, img.rows, img.step, QImage::Format_RGB888); //三通道
//    QPixmap pix = QPixmap::fromImage(image.scaled(ui->lab_show_img->width(), ui->lab_show_img->height(), Qt::KeepAspectRatio));
//    ui->lab_show_img->setPixmap(pix);
//    ui->lab_show_img->show();
}

void Widget::get_mask(char *get_buf)
{
    /*异物显示为rgb全白，再与原始图像叠加，白色像素代表异物*/
    std::vector<cv::Mat> mask_rgb;
    cv::Mat mask_show = cv::Mat(REALHEIGHT, REALWIDTH, CV_8UC3);
    cv::Mat mask = cv::Mat(REALHEIGHT, REALWIDTH, CV_8UC1, get_buf);
    mask *= 255;
    mask_rgb.push_back(mask);
    mask_rgb.push_back(mask);
    mask_rgb.push_back(mask);
    cv::merge(mask_rgb, mask_show);
    cv::Mat add_img = cv::Mat(REALHEIGHT, REALWIDTH, CV_8UC3);
    cv::add(img, mask_show, add_img);

    /*当时是为了保证传送带显示纯黑，实际不需要用到
    cv::rotate(mask_show, mask_show, cv::ROTATE_90_CLOCKWISE);
    cv::Mat img_show = img + mask_show;
    cv::Mat img_show = mask_show;
    */

    const unsigned char *pSrc_img = (const unsigned char*)add_img.data;
    QImage image_img(pSrc_img, add_img.cols, add_img.rows, add_img.step, QImage::Format_RGB888); //三通道
    QPixmap pix_img = QPixmap::fromImage(image_img.scaled(ui->lab_show_img->width(), ui->lab_show_img->height(), Qt::KeepAspectRatio));
    ui->lab_show_img->setPixmap(pix_img);
    ui->lab_show_img->show();

    const unsigned char *pSrc_mask = (const unsigned char*)mask_show.data;
    QImage image_mask(pSrc_mask, mask_show.cols, mask_show.rows, mask_show.step, QImage::Format_RGB888);
    QPixmap pix_mask = QPixmap::fromImage(image_mask.scaled(ui->lab_show_mask->width(),ui->lab_show_mask->height(),Qt::KeepAspectRatio));
    ui->lab_show_mask->setPixmap(pix_mask);
    ui->lab_show_mask->show();

    /*
    //灰度图显示，白色为杂质，取消了RGB显示功能，有利于速度提升，内存减少占用
    //修改于2022年6月21日
    cv::Mat mask = cv::Mat(REALHEIGHT, REALWIDTH, CV_8UC1, get_buf);
    mask *= 255;
    const unsigned char *pSrc = (const unsigned char*)mask.data;
    QImage image(pSrc, mask.cols, mask.rows, mask.step, QImage::Format_Grayscale8);
    QPixmap pix = QPixmap::fromImage(image.scaled(ui->lab_show_mask->width(),ui->lab_show_mask->height(),Qt::KeepAspectRatio));
    ui->lab_show_mask->setPixmap(pix);
    ui->lab_show_mask->show();
    */
}

void Widget::get_valve_data(uint8_t *valve_data)
{
    uint8_t* mask_buf = new uint8_t[32 * REALHEIGHT + 8];
    mask_buf[0] = 0xAA;
    //mask_buf[1]和mask_buf[2]表示数据长度
    mask_buf[1] = 0x20;
    mask_buf[2] = 0x02;
    mask_buf[3] = 'd';
    mask_buf[4] = 'a';
    memcpy(mask_buf + 5, valve_data, 32*REALHEIGHT);
    mask_buf[32*256+5] = 0xFF;
    mask_buf[32*256+6] = 0xFF;
    mask_buf[32*256+7] = 0xBB;

    if(lower_machine != nullptr)
    {
        lower_machine->write((const char*)mask_buf, REALHEIGHT * 32 + 8);
        lower_machine->flush();
        cout << ">>> send to guoyiren successfully! >>>" << endl;
    }

    delete [] mask_buf;
}


void Widget::get_send_to_up_machine(float* sendbuf)
{
    cout << "suuuuuuuu" << endl;
    if(connect_to_monitor && send_to_up_machine)
    {
        cout << REALHEIGHT*REALWIDTH*valid.size()*sizeof(float) << endl;
        uint32_t len = up_machine->write((const char*)sendbuf, REALHEIGHT*REALWIDTH*valid.size()*sizeof(float));
        up_machine->flush();
        cout << ">>> send img finished! >>>" << len << endl;
    }
}

void Widget::readfrom_up_machine()
{
    cout << "receive!!!!!!!!!!!!" << endl;
    char recvbuf[10] = {0};
    up_machine->read(recvbuf, 10);
    if(strcmp(recvbuf, "1") == 0)
    {
        send_to_up_machine = true;
        cout << "receive 1" << endl;
    }
    else if(strcmp(recvbuf, "0") == 0)
    {
        send_to_up_machine = false;
        cout << "receive 0" << endl;
    }
}

void Widget::On_btn_sendsingle_clicked()
{
    int val = ui->spinBox_channel->value() - 1;
    QString valve_x = QString::fromStdString(to_string(val));
    int len_valve_x = valve_x.size();
    QByteArray valve_byte = valve_x.toLatin1();
    uint8_t* sendbuf = new uint8_t[len_valve_x + 8];
    sendbuf[0] = 0xAA;
    sendbuf[1] = 0x00;
    sendbuf[2] = len_valve_x + 2;
    sendbuf[3] = 't';
    sendbuf[4] = 'e';
    memcpy(sendbuf + 5, valve_byte.data(), len_valve_x);
    sendbuf[len_valve_x + 5] = 0xFF;
    sendbuf[len_valve_x + 6] = 0xFF;
    sendbuf[len_valve_x + 7] = 0xBB;
    if(lower_machine != nullptr)
    {
        lower_machine->write((const char*)sendbuf, len_valve_x + 8);
        cout << "send single " << val+1 << " success" << endl;
    }
    delete [] sendbuf;
}

void Widget::On_btn_autosend_clicked()
{
    uint8_t start_test[11] = {0};
    start_test[0] = 0xAA;
    start_test[1] = 0x00;
    start_test[2] = 0x05;
    start_test[3] = 't';
    start_test[4] = 'e';
    start_test[5] = '2';
    start_test[6] = '5';
    start_test[7] = '7';
    start_test[8] = 0xFF;
    start_test[9] = 0xFF;
    start_test[10] = 0xBB;
    if(lower_machine != nullptr)
    {
        lower_machine->write((const char*)start_test, 11);
        cout << "send autosend success" << endl;
    }
}

void Widget::on_btn_1_clicked()
{
    parameter->password += "1";
    int len = parameter->password.size();
    ui->line_password->setText(QString(len, '*'));
}

void Widget::on_btn_2_clicked()
{
    parameter->password += "2";
    int len = parameter->password.size();
    ui->line_password->setText(QString(len, '*'));
}

void Widget::on_btn_3_clicked()
{
    parameter->password += "3";
    int len = parameter->password.size();
    ui->line_password->setText(QString(len, '*'));
}

void Widget::on_btn_4_clicked()
{
    parameter->password += "4";
    int len = parameter->password.size();
    ui->line_password->setText(QString(len, '*'));
}

void Widget::on_btn_5_clicked()
{
    parameter->password += "5";
    int len = parameter->password.size();
    ui->line_password->setText(QString(len, '*'));
}

void Widget::on_btn_6_clicked()
{
    parameter->password += "6";
    int len = parameter->password.size();
    ui->line_password->setText(QString(len, '*'));
}

void Widget::on_btn_7_clicked()
{
    parameter->password += "7";
    int len = parameter->password.size();
    ui->line_password->setText(QString(len, '*'));
}

void Widget::on_btn_8_clicked()
{
    parameter->password += "8";
    int len = parameter->password.size();
    ui->line_password->setText(QString(len, '*'));
}

void Widget::on_btn_9_clicked()
{
    parameter->password += "9";
    int len = parameter->password.size();
    ui->line_password->setText(QString(len, '*'));
}

void Widget::on_btn_0_clicked()
{
    parameter->password += "0";
    int len = parameter->password.size();
    ui->line_password->setText(QString(len, '*'));
}

void Widget::on_btn_clear_clicked()
{
    parameter->password.clear();
    ui->line_password->setText("");
}

void Widget::on_btn_del_clicked()
{
    int len = parameter->password.size();
    if(len > 0){
        parameter->password.pop_back();
        --len;
        ui->line_password->setText(QString(len, '*'));
    }
}

void Widget::on_btn_ensure_clicked()
{
    if(parameter->password != parameter->file_info[1]){
        //密码错误
        QMessageBox::information(this, "提示", "密码错误！");
        parameter->password.clear();
        ui->line_password->setText("");
    }
    else{
        //密码正确
        parameter->password.clear();
        ui->line_password->setText("");
        ui->tabWidget->setCurrentIndex(2);
    }
}

void Widget::on_btn_back_clicked()
{
    parameter->password.clear();
    ui->tabWidget->setCurrentIndex(0);
}

void Widget::on_btn_send_plus_clicked()
{
    int send_val = 0;
    int val = ui->spinBox_channel->value();
    send_val = val - 1;
    QString valve_x = QString::fromStdString(to_string(send_val));
    int len_valve_x = valve_x.size();
    QByteArray valve_byte = valve_x.toLatin1();
    uint8_t* sendbuf = new uint8_t[len_valve_x + 8];
    sendbuf[0] = 0xAA;
    sendbuf[1] = 0x00;
    sendbuf[2] = len_valve_x + 2;
    sendbuf[3] = 't';
    sendbuf[4] = 'e';
    memcpy(sendbuf + 5, valve_byte.data(), len_valve_x);
    sendbuf[len_valve_x + 5] = 0xFF;
    sendbuf[len_valve_x + 6] = 0xFF;
    sendbuf[len_valve_x + 7] = 0xBB;
    if(lower_machine != nullptr)
    {
        lower_machine->write((const char*)sendbuf, len_valve_x + 8);
        cout << "send single " << val << " success" << endl;
    }
    val += 1;
    if(val == 257)
        val = 1;
    ui->spinBox_channel->setValue(val);
    delete [] sendbuf;
}

void Widget::on_btn_send_min_clicked()
{
    int send_val = 0;
    int val = ui->spinBox_channel->value();
    send_val = val - 1;
    QString valve_x = QString::fromStdString(to_string(send_val));
    int len_valve_x = valve_x.size();
    QByteArray valve_byte = valve_x.toLatin1();
    uint8_t* sendbuf = new uint8_t[len_valve_x + 8];
    sendbuf[0] = 0xAA;
    sendbuf[1] = 0x00;
    sendbuf[2] = len_valve_x + 2;
    sendbuf[3] = 't';
    sendbuf[4] = 'e';
    memcpy(sendbuf + 5, valve_byte.data(), len_valve_x);
    sendbuf[len_valve_x + 5] = 0xFF;
    sendbuf[len_valve_x + 6] = 0xFF;
    sendbuf[len_valve_x + 7] = 0xBB;
    if(lower_machine != nullptr)
    {
        lower_machine->write((const char*)sendbuf, len_valve_x + 8);
        cout << "send single " << val << " success" << endl;
    }
    val -= 1;
    if(val == 0)
        val = 256;
    ui->spinBox_channel->setValue(val);
    delete [] sendbuf;
}

void Widget::time_log(int state)
{
    ofstream logfile;
    logfile.open(log_file, ios::app);
    QDateTime current_time = QDateTime::currentDateTime();
    string str = current_time.toString("yyyy MM.dd hh:mm:ss").toStdString();
    if(state == 1)
        logfile << str << " On" << endl;
    else
        logfile << str << " Off" << endl;
    logfile.close();
}

void Widget::On_btn_threshold_clicked()
{
    file_threshold = ui->spinbox_threshold->value();
    int fd = open("/tmp/dkimg.fifo", O_WRONLY);
    string str = to_string(file_threshold);
    int ret = write(fd, str.c_str(), str.size());
    if(ret > 0)
        cout << "threshold send success  " << file_threshold << endl;

    file_threshold_rgb = ui->spinbox_threshold_rgb->value();
    fd = open("/tmp/dkrgb.fifo", O_WRONLY);
    str = to_string(file_threshold_rgb);
    ret = write(fd, str.c_str(), str.size());
    if(ret > 0)
        cout << "threshold send success  " << file_threshold_rgb << endl;

}

void Widget::on_btn_set_lower_clicked()
{
    file_delay = ui->spinbox_delaytime->value();
    file_encoder = ui->spinbox_encoder->value();
    file_valve = ui->spinbox_valve->value();
}

void Widget::on_btn_set_camera_clicked()
{
    file_explosure = ui->spinbox_explosure->value();
    file_delay = ui->spinbox_delaytime->value();
    file_encoder = ui->spinbox_encoder->value();
    file_valve = ui->spinbox_valve->value();
    file_mroi = ui->lineEdit_MROI->text().toStdString();
    file_valid = ui->lineEdit_VALID->text().toStdString();
    R = ui->spinbox_R->value();
    G = ui->spinbox_G->value();
    B = ui->spinbox_B->value();
    file_threshold = ui->spinbox_threshold->value();
    file_threshold_rgb = ui->spinbox_threshold_rgb->value();

    /*
    vector<string> config;
    config.push_back(to_string(file_explosure));
    config.push_back(to_string(file_encoder));
    config.push_back(to_string(file_delay));
    config.push_back(to_string(file_valve));
    config.push_back(file_mroi);
    config.push_back(to_string(R));
    config.push_back(to_string(G));
    config.push_back(to_string(B));
    config.push_back(file_valid);
    config.push_back(to_string(file_threshold));

    ofstream outfile;
    outfile.open(parameter_file, ios::out | ios::trunc);
    for(auto s : config)
    {
        outfile << s << endl;
    }
    */
}

void Widget::on_btn_set_clicked()
{
    //保存配置文件至本地
    file_explosure = ui->spinbox_explosure->value();
    file_delay = ui->spinbox_delaytime->value();
    file_encoder = ui->spinbox_encoder->value();
    file_valve = ui->spinbox_valve->value();
    file_mroi = ui->lineEdit_MROI->text().toStdString();
    file_valid = ui->lineEdit_VALID->text().toStdString();
    R = ui->spinbox_R->value();
    G = ui->spinbox_G->value();
    B = ui->spinbox_B->value();
    file_threshold = ui->spinbox_threshold->value();
    file_threshold_rgb = ui->spinbox_threshold_rgb->value();

    vector<string> config;
    config.push_back(to_string(file_explosure));
    config.push_back(to_string(file_encoder));
    config.push_back(to_string(file_delay));
    config.push_back(to_string(file_valve));
    config.push_back(file_mroi);
    config.push_back(to_string(R));
    config.push_back(to_string(G));
    config.push_back(to_string(B));
    config.push_back(file_valid);
    config.push_back(to_string(file_threshold));
    config.push_back(to_string(file_threshold_rgb));

    ofstream outfile;
    outfile.open(parameter_file, ios::out | ios::trunc);
    for(auto s : config)
    {
        outfile << s << endl;
    }
}

void Widget::on_btn_rgb_white_clicked()
{
    //    //设置触发模式为 外触发，线路Line0，上升沿触发
    PvResult lResult = rgb_camera->parameter_array->SetEnumValue("LineTriggerMode", "Off");
    if(!lResult.IsOK()){
        cout << "set triggermode failed" << endl;
        return;
    }
    lResult = rgb_camera->parameter_array->SetFloatValue("AcquisitionLineRate", 4800.0f);
    lResult = rgb_camera->Whitecal->Execute();
    sleep(2);
}

void Widget::on_btn_rgb_black_clicked()
{
    //    //设置触发模式为 外触发，线路Line0，上升沿触发
    PvResult lResult = rgb_camera->parameter_array->SetEnumValue("LineTriggerMode", "Off");
    if(!lResult.IsOK()){
        cout << "set triggermode failed" << endl;
        return;
    }
    lResult = rgb_camera->parameter_array->SetFloatValue("AcquisitionLineRate", 4800.0f);
    lResult = rgb_camera->Blackcal->Execute();
    sleep(2);
}

void Widget::on_btn_save_enable_clicked()
{
    send_thread->save_flag = true;
    ui->btn_save_enable->setEnabled(false);
    ui->btn_save_disenable->setEnabled(true);
}

void Widget::on_btn_save_disenable_clicked()
{
    send_thread->save_flag = false;
    ui->btn_save_enable->setEnabled(true);
    ui->btn_save_disenable->setEnabled(false);

}
