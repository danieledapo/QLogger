QT       -= gui
QT       += network
CONFIG   += c++11

TARGET = QLogger
TEMPLATE = lib

DEFINES += QLOGGER_LIBRARY

# Uncomment the following line to disable debug and warning output
# Debug can be useful to detect bugs, so be careful.
# DEFINES += QT_NO_DEBUG_OUTPUT QT_NO_WARNING_OUTPUT

SOURCES += qlogger.cpp

HEADERS += qlogger.h\
        qlogger_global.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
