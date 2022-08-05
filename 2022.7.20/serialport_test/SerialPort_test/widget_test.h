#ifndef WIDGET_TEST_H
#define WIDGET_TEST_H

#include <QMainWindow>
#include <QtSerialPort/QtSerialPort>
#include <QList>
#include <QtSerialPort/QSerialPortInfo>


namespace Ui {
class widget_test;
}

class widget_test : public QMainWindow
{
    Q_OBJECT

public:
    explicit widget_test(QWidget *parent = 0);
    ~widget_test();

private:
    Ui::widget_test *ui;
};

#endif // WIDGET_TEST_H
