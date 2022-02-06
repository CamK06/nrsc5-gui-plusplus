#pragma once

#include <QMainWindow>
extern "C" {
#include <nrsc5.h>
}

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
    nrsc5_t *radio = NULL;
    bool playing = false;
    double freq;
};