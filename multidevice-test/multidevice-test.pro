QT       += core gui serialport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11 c++14 c++17 c++2a c++2b
VERSION = 1.0.1.0
DEFINES += \
    APP_NAME=\\\"$$TARGET\\\" \
    APP_VERSION=\\\"$$VERSION\\\"

INCLUDEPATH += ../src

SOURCES += \
    ../src/qmultidevice.cpp \
    main.cpp

HEADERS +=  \
    mainwindow.hpp \
    ../src/qmultidevice.h \
    ../src/qserialport_widget.hpp \
    ../src/qtcpclient_widget.hpp \
    ../src/qtcpserver_widget.hpp \
    ../src/qudpsocket_widget.hpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

