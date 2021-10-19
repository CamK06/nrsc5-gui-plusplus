QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = nrsc5-gui
TEMPLATE = app

SOURCES += src/main.cpp src/mainwindow.cpp
HEADERS += src/mainwindow.h
FORMS += src/mainwindow.ui