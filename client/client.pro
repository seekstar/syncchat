#-------------------------------------------------
#
# Project created by QtCreator 2020-04-28T21:34:12
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = client
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ../include

SOURCES += ../src/*\
        main.cpp\
        mainwindow.cpp \
    winlogin.cpp \
    dialogsignup.cpp \
    myodbc.cpp \
    dialogreconnect.cpp \
    mainmanager.cpp \
    myglobal.cpp \
    pushbuf.cpp \
    dialogaddfriend.cpp \
    dialogcreategroup.cpp \
    dialogjoingroup.cpp \
    sslmanagerbase.cpp \
    sslmanagerdispatch.cpp \
    sslmanageruser.cpp \
    sslmanagerfriend.cpp \
    sslmanagermsg.cpp \
    sslmanagergrp.cpp \
    sslmanagergrpmsg.cpp \
    widgetmoment.cpp \
    widgetcomment.cpp \
    dialogcomments.cpp \
    dialogmoments.cpp \
    dialogeditmoment.cpp \
    sslmanagermoment.cpp \
    dialogfindbyusername.cpp

HEADERS  += ../include/*\
    mainwindow.h \
    winlogin.h \
    dialogsignup.h \
    myodbc.h \
    dialogreconnect.h \
    mainmanager.h \
    sslmanager.h \
    myglobal.h \
    pushbuf.h \
    dialogaddfriend.h \
    clienttypes.h \
    dialogcreategroup.h \
    dialogjoingroup.h \
    sslmanagerbase.h \
    widgetmoment.h \
    widgetcomment.h \
    dialogcomments.h \
    dialogmoments.h \
    dialogeditmoment.h \
    dialogfindbyusername.h

FORMS    += mainwindow.ui \
    winlogin.ui \
    dialogsignup.ui \
    dialogreconnect.ui \
    dialogaddfriend.ui \
    dialogcreategroup.ui \
    dialogjoingroup.ui \
    widgetmoment.ui \
    widgetcomment.ui \
    dialogcomments.ui \
    dialogmoments.ui \
    dialogeditmoment.ui \
    dialogfindbyusername.ui

win32 {
    DEFINES  -= UNICODE
    LIBS += -lodbc32
}
unix {
    LIBS += -lodbc
}
LIBS += -lboost_system -lssl -lcrypto
