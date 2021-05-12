QT += core
QT -= gui
QT += network
QT += sql

CONFIG += c++11

TARGET = 12306server
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    server.cpp \
    serverthread.cpp

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
    server.h \
    serverthread.h

INCLUDEPATH += "C:\Program Files (x86)\PostgreSQL\10\include"
LIBS += "C:\Program Files (x86)\PostgreSQL\10\lib\libpq.lib"

#INCLUDEPATH += "C:\Program Files\PostgreSQL\13\include"
#LIBS += "C:\Program Files\PostgreSQL\13\lib\libpq.lib"
