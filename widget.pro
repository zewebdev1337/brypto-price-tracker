QT       += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = price-tracker
TEMPLATE = app
CONFIG += c++17
DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += /usr/include/qt

SOURCES += \
    src/main.cpp \
    src/widget.cpp

HEADERS += \
    src/widget.hpp
