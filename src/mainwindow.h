#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QString>
extern "C"
{
#include <nrsc5.h>
}
#include <portaudio.h>
#include <thread>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    void setTrack(const char *track, const char *artist, const char *album, const char *genre);
    void setStation(const char *name, const char *slogan);
    void setPicture(int service, const uint8_t* data, unsigned int size);
    void setLogo(int service);
    void updateChannelButtons();
    void audio_worker();

    // Radio
    bool playing = false;
    int currentProgram = 0;
    float errorRate = 0;
    int picturePorts[5] = {0, 0, 0, 0, 0};
    int logoPorts[5] = {0, 0, 0, 0, 0};
    int numAudioServices = 1;
    std::string stationName;

    // Audio
    int audio_packets = 0;
    int audio_bytes = 0;
    float audio_kbps = 0;

private:
    void play();
    static void radioCallback(const nrsc5_event_t *evt, void *opaque);

    // For managing the vector when playing samples
    // taken from stackoverflow, could probably be
    // put into another file instead of here.
    template <typename T>
    static void pop_front(std::vector<T> &vec)
    {
        assert(!vec.empty());
        vec.erase(vec.begin());
    }

    // GUI
    Ui::MainWindow *ui;
    QLabel statusBarLabel;
    
    // Radio
    nrsc5_t *radio = NULL;
    double freq;

    // Audio
    PaStream *stream = NULL;
    std::vector<uint16_t> audioBuffer;
    std::thread audioThread;

private slots:
    void setChannel(int channel);
};