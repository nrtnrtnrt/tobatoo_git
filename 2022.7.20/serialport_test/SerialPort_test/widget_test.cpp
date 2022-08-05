#include "widget_test.h"
#include "ui_widget_test.h"

widget_test::widget_test(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::widget_test)
{
    ui->setupUi(this);

    QList<QSerialPortInfo> list  = QSerialPortInfo::availablePorts();
    foreach (const QSerialPortInfo & info, list) {
            ui->comboBox->addItem(info.portName());
    }

}

widget_test::~widget_test()
{
    delete ui;
}
