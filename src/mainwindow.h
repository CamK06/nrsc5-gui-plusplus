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
    void setTrack(const char* track, const char* artist);
    void setStation(const char* name, const char* slogan);

    int currentProgram = 0;

private:
    void play();
    static void radioCallback(const nrsc5_event_t *evt, void *opaque);

    Ui::MainWindow *ui;
    nrsc5_t *radio = NULL;
    bool playing = false;
    double freq;
};