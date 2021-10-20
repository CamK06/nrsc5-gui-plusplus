#pragma once

#include <QMainWindow>

#include "nrsc.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private:
    void play();

    Ui::MainWindow *ui;
    NRSC5 *radio;
    bool playing = false;
    double freq;
};