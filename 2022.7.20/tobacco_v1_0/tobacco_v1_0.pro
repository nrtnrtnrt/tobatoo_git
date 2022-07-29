#-------------------------------------------------
#
# Project created by QtCreator 2022-03-06T19:05:49
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = tobacco_v1_0
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

UI_DIR += ./

SOURCES += \
        main.cpp \
        widget.cpp \
    camera.cpp \
    thread.cpp \
    parameter.cpp

HEADERS += \
        widget.h \
    camera.h \
    thread.h \
    parameter.h

FORMS += \
        widget.ui


LIBS += -L/opt/pleora/ebus_sdk/Ubuntu-18.04-x86_64/lib/ -lEbTransportLayerLib \
                                                    -lPtUtilsLib \
                                                    -lPvBase \
                                                    -lPvCameraBridge \
                                                    -lPvGenICam \
                                                    -lPvPersistence \
                                                    -lPvStream \
                                                    -lPvTransmitter\
                                                    -lSimpleImagingLib \
                                                    -lEbUtilsLib \
                                                    -lPtConvertersLib \
                                                    -lPvAppUtils \
                                                    -lPvBuffer \
                                                    -lPvDevice \
                                                    -lPvGUI \
                                                    -lPvSerial \
                                                    -lPvSystem \
                                                    -lPvVirtualDevice

INCLUDEPATH += /opt/pleora/ebus_sdk/Ubuntu-18.04-x86_64/include
DEPENDPATH += /opt/pleora/ebus_sdk/Ubuntu-18.04-x86_64/include


LIBS += -L/usr/lib/x86_64-linux-gnu/ -lopencv_core \
                                     -lopencv_imgproc \
                                     -lopencv_highgui \
                                     -lopencv_imgcodecs \

INCLUDEPATH += /usr/include/opencv2
DEPENDPATH += /usr/include/opencv2

