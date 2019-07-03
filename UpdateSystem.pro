#-------------------------------------------------
#
# Project created by QtCreator 2019-05-13T21:47:39
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
#TAGET的名字不能以Updata为关键字
TARGET = AppTest
TEMPLATE = app

SOURCES += main.cpp\
        UpdateSystem.cpp

HEADERS  += UpdateSystem.h

FORMS    += UpdateSystem.ui \
    authenticationdialog.ui

RESOURCES += \
    images.qrc

RC_FILE = ExeVersion.rc\

CONFIG   += c++11

win32:LIBS+=-lVersion
