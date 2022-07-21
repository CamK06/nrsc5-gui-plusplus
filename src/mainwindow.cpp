#include <QMainWindow>
#include <QMessageBox>
#include <QObject>
#include <QSpinBox>
#include <QPixmap>
#include <thread>
#include <filesystem>
#include <spdlog/spdlog.h>
extern "C" {
#include <nrsc5.h>
}

#include "mainwindow.h"
#include "version.h"
#include "./ui_mainwindow.h"
#include <portaudio.h>
#include <fstream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{ 
    spdlog::info(PROGRAM " " VERSION);
    ui->setupUi(this);
    statusBarLabel.setAlignment(Qt::AlignRight);
    statusBarLabel.setText(QString("BER: %1 | Kbps: %2").arg(errorRate).arg(audio_kbps));
    ui->statusBar->addWidget(&statusBarLabel, 1);

    // Set up the image display
    //ui->albumArtImage->setScaledContents(true);
    ui->albumArtImage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    // Signal handling
    connect(ui->playButton, &QPushButton::pressed, this, &MainWindow::play);
    connect(ui->frequencySelect, SIGNAL(valueChanged(int)), this, SLOT(setChannel(int)));
    connect(ui->hd1Button, &QPushButton::pressed, this, [=]() {setChannel(1);});
    connect(ui->hd2Button, &QPushButton::pressed, this, [=]() {setChannel(2);});
    connect(ui->hd3Button, &QPushButton::pressed, this, [=]() {setChannel(3);});
    connect(ui->hd4Button, &QPushButton::pressed, this, [=]() {setChannel(4);});

    spdlog::info("Initialized main window");
}

void MainWindow::setChannel(int channel)
{
    // Update the logo
    setLogo(channel-1);
    currentProgram = channel-1;
    updateChannelButtons();

    // TODO: Make this reset audio
}

void MainWindow::play()
{
    if(!playing) {

        spdlog::info("Playing...");
        playing = true;

        // Parse the frequency
        freq = ui->frequencySelect->value();
        if(freq == 0) { // Check if the string parsed correctly
            QMessageBox::warning(this, "Warning", "The entered frequency is invalid!", QMessageBox::Ok);
            return;
        }
        else if(freq < 88 || freq > 108) {
            QMessageBox::warning(this, "Warning", "The entered frequency is outside of the FM band! Frequency must be 88-108MHz");
            return;
        }
        spdlog::info("Frequency (MHz): {}", freq);
        freq *= 1000000;
        spdlog::info("Frequency (Hz): {}", freq);

        // Radio setup
        if(nrsc5_open(&radio, 0) != 0) { // TODO: Don't just assume device 0 (this will be a GUI option later)
            spdlog::error("Failed to open SDR!");
            QMessageBox::warning(this, "Error", "Failed to open SDR! Exiting...", QMessageBox::Ok);
            exit(-1);
        }
        nrsc5_set_frequency(radio, freq);
        nrsc5_set_bias_tee(radio, true);
        nrsc5_set_callback(radio, radioCallback, this);
        nrsc5_start(radio);

        // Adjust the UI
        ui->frequencySelect->setDisabled(true);
        ui->playButton->setText("Stop");

        // Initialize portaudio
        PaError err = Pa_Initialize();
        if(err != paNoError) {
            spdlog::error("Failed to initialize portaudio!");
            QMessageBox::warning(this, "Error", "Failed to initialize portaudio! Exiting...", QMessageBox::Ok);
            exit(-1);
        }

        // Create the audio worker thread
        audioThread = std::thread(&MainWindow::audio_worker, this);
    }
    else {

        spdlog::info("Stopping...");
        playing = false;

        // Stop the audio worker
        audioThread.join();
        audioBuffer.clear();

        // Stop the radio
        nrsc5_stop(radio);
        nrsc5_set_bias_tee(radio, false);
        nrsc5_close(radio);

        // Adjust the UI
        ui->frequencySelect->setDisabled(false);
        ui->playButton->setText("Play");
        ui->albumArtImage->setPixmap(QPixmap());

        // General cleanup
        numAudioServices = 1;
        audio_packets = 0;
        audio_kbps = 0;
        errorRate = 0;
        ui->hd1Button->setEnabled(false);
        ui->hd2Button->setEnabled(false);
        ui->hd3Button->setEnabled(false);
        ui->hd4Button->setEnabled(false);
        statusBarLabel.setText(QString("BER: %1 | Kbps: %2").arg(errorRate).arg(audio_kbps));
        ui->titleText->setText("");
        ui->artistText->setText("");
        ui->albumText->setText("");
        ui->genreText->setText("");
    }
}

void MainWindow::radioCallback(const nrsc5_event_t *evt, void *opaque)
{
    MainWindow* mainWindow = (MainWindow*)opaque;
    switch(evt->event) {

        // ID3 metadata
        case NRSC5_EVENT_ID3:
            if(evt->id3.program == mainWindow->currentProgram)
                mainWindow->setTrack(evt->id3.title, evt->id3.artist, evt->id3.album, evt->id3.genre);
            break;

        // Station information
        case NRSC5_EVENT_SIS:
            mainWindow->setStation(evt->sis.name, evt->sis.slogan);
            mainWindow->setLogo(0);
            
            if(mainWindow->numAudioServices > 1) // We've already found the number of services
                return;
            nrsc5_sis_asd_t *audio_service;
            for (audio_service = evt->sis.audio_services; audio_service != NULL; audio_service = audio_service->next)
                mainWindow->numAudioServices++;

            // Enable the HD buttons according to the numAudioServices value
            // This is probably messy but meh... it works..?
            mainWindow->updateChannelButtons();
            // if(mainWindow->numAudioServices < 2)
            //     mainWindow->ui->hd1Button->setEnabled(true);
            // if(mainWindow->numAudioServices < 3)
            //     mainWindow->ui->hd2Button->setEnabled(true);
            // if(mainWindow->numAudioServices < 4)
            //     mainWindow->ui->hd3Button->setEnabled(true);
            // else
            //     mainWindow->ui->hd4Button->setEnabled(true);
            break;

        case NRSC5_EVENT_SIG:
            // TODO: Clean this lmao
            nrsc5_sig_service_t *service;
            for(service = evt->sig.services; service != NULL; service = service->next) {
                if(service->type == NRSC5_SIG_SERVICE_AUDIO) {
                    nrsc5_sig_component_t *component;
                    for(component = service->components; component != NULL; component = component->next) {
                        if(component->type == NRSC5_SIG_COMPONENT_DATA) {
                            if(component->data.mime == NRSC5_MIME_PRIMARY_IMAGE) {
                                mainWindow->picturePorts[service->number-1] = component->data.port;
                            }
                            else if(component->data.mime == NRSC5_MIME_STATION_LOGO) {
                                mainWindow->logoPorts[service->number-1] = component->data.port;
                            }
                        }
                    }
                }
            }
            break;

        case NRSC5_EVENT_LOT:
            for(int i = 0; i < mainWindow->numAudioServices; i++) {
                if(evt->lot.port == mainWindow->picturePorts[i]) {
                    if(i == mainWindow->currentProgram) {
                        mainWindow->setPicture(i, evt->lot.data, evt->lot.size);
                    }
                    break;
                }
                else if(evt->lot.port == mainWindow->logoPorts[i]) {

                    // Check if the logos directory exists
                    // TODO: make this in /usr/share or whatever
                    std::filesystem::path logosPath = std::filesystem::current_path() / "logos";
                    if(!std::filesystem::exists(logosPath))
                        std::filesystem::create_directory(logosPath);

                    // Write the logo to a file
                    std::filesystem::path logoPath = logosPath / QString("%1 - %2.jpg").arg(mainWindow->stationName.c_str()).arg(i).toStdString();
                    std::ofstream logoFile(logoPath);
                    logoFile.write((const char*)evt->lot.data, evt->lot.size);
                    logoFile.close();

                    if(i == mainWindow->currentProgram)
                        mainWindow->setLogo(i);
                    break;
                }
            }
            break;

        // Audio stream
        case NRSC5_EVENT_AUDIO:
            // Push the audio data to the buffer
            if(evt->audio.program != mainWindow->currentProgram)
                break;

            // Push the samples to the audio buffer
            for(int i = 0; i < evt->audio.count; i++)
                mainWindow->audioBuffer.push_back(evt->audio.data[i]);
            break;

        // HDC audio packet
        case NRSC5_EVENT_HDC:
            if(evt->hdc.program != mainWindow->currentProgram)
                break;

            mainWindow->audio_packets++;
            mainWindow->audio_bytes += evt->hdc.count * sizeof(evt->hdc.data[0]);
            if(mainWindow->audio_packets >= 32) {
                // Calculate kbps
                mainWindow->audio_kbps = ((float)mainWindow->audio_bytes * 8 * 44100 / 2048 / mainWindow->audio_packets / 1000);
                mainWindow->audio_packets = 0;
                mainWindow->audio_bytes = 0;
            }
            break;

        // Bit error rate
        case NRSC5_EVENT_BER:
            mainWindow->errorRate = evt->ber.cber;
            break;

        default:
            break;
    }

    // Update the status bar with BER and audio kbps
    mainWindow->statusBarLabel.setText(QString("BER: %1 | Kbps: %2").arg(mainWindow->errorRate).arg(mainWindow->audio_kbps));
}

void MainWindow::setTrack(const char* track, const char* artist, const char* album, const char* genre)
{
    ui->titleText->setText(track);
    // YANDEV GO BRRRRRRR
    if(artist != nullptr)
        ui->artistText->setText(artist);
    else
        ui->artistText->setText("N/A");
    if(album != nullptr)
        ui->albumText->setText(album);
    else
        ui->albumText->setText("N/A");
    if(genre != nullptr)
        ui->genreText->setText(genre);
    else
        ui->genreText->setText("N/A");
}

void MainWindow::setStation(const char* name, const char* slogan)
{
    // TODO: Set station info
    // if(slogan != nullptr)
    //     ui->callsignLab->setText(QString("%1 - %2").arg(name, slogan));
    // else
    //     ui->callsignLab->setText(name);

    stationName = name;
}

void MainWindow::setLogo(int service)
{   
    // Get the logo file
    std::filesystem::path logosPath = std::filesystem::current_path() / "logos";
    std::filesystem::path logoPath = logosPath / QString("%1 - %2.jpg").arg(stationName.c_str()).arg(service).toStdString();
    if(!std::filesystem::exists(logoPath))
        return;

    // Set the logo
    QPixmap logo(logoPath.c_str());
    ui->albumArtImage->setPixmap(logo);
}

void MainWindow::setPicture(int service, const uint8_t* data, unsigned int size)
{
    if(size == 0)
        return;
    
    // TODO: Implement
}

void MainWindow::updateChannelButtons()
{
    // Disable all channel buttons
    ui->hd1Button->setEnabled(false);
    ui->hd2Button->setEnabled(false);
    ui->hd3Button->setEnabled(false);
    ui->hd4Button->setEnabled(false);

    // This is probably a bad way to do this but meh idgaf
    for(int i = 0; i < 4; i++) {
        if(i == 0 && currentProgram != i && i < numAudioServices)
            ui->hd1Button->setEnabled(true);
        else if(i == 1 && currentProgram != i && i < numAudioServices)
            ui->hd2Button->setEnabled(true);
        else if(i == 2 && currentProgram != i && i < numAudioServices)
            ui->hd3Button->setEnabled(true);
        else if(i == 3 && currentProgram != i && i < numAudioServices)
            ui->hd4Button->setEnabled(true);
    }
}

void MainWindow::audio_worker()
{
    // Write samples from the audio buffer using the port audio blocking API
    PaStreamParameters outputParameters;
    outputParameters.device = Pa_GetDefaultOutputDevice();
    outputParameters.channelCount = 2;
    outputParameters.sampleFormat = paInt16;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = nullptr;
    PaStream* audioStream = NULL;
    PaError err = Pa_OpenStream(&audioStream, nullptr, &outputParameters, 44100, 2048, paNoFlag, NULL, NULL);
    if(err != paNoError) {
        spdlog::error("Failed to open audio stream!");
        QMessageBox::warning(this, "Error", "Failed to open audio stream! Exiting...", QMessageBox::Ok);
        exit(-1);
    }
    err = Pa_StartStream(audioStream);
    if(err != paNoError) {
        spdlog::error("Failed to start audio stream!");
        QMessageBox::warning(this, "Error", "Failed to start audio stream! Exiting...", QMessageBox::Ok);
        exit(-1);
    }
    
    int16_t* sample = new int16_t[128];
    while(playing) {
        if(audioBuffer.size() > 0) {
            for(int i = 0; i < 128; i++) {
                if(audioBuffer.size() > 0) {
                    sample[i] = audioBuffer.front();
                    pop_front(audioBuffer);
                }
                else
                    sample[i] = 0;
            }
            err = Pa_WriteStream(audioStream, sample, 64);
            if(err != paNoError) {
                spdlog::error("Failed to write audio stream!");
                QMessageBox::warning(this, "Error", "Failed to write audio stream! Exiting...", QMessageBox::Ok);
                exit(-1);
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // Clean up
    err = Pa_StopStream(audioStream);
    if(err != paNoError) {
        spdlog::error("Failed to stop audio stream!");
        QMessageBox::warning(this, "Error", "Failed to stop audio stream! Exiting...", QMessageBox::Ok);
        exit(-1);
    }
    err = Pa_CloseStream(audioStream);
    if(err != paNoError) {
        spdlog::error("Failed to close audio stream!");
        QMessageBox::warning(this, "Error", "Failed to close audio stream! Exiting...", QMessageBox::Ok);
        exit(-1);
    }
    delete[] sample;
}