#include <QMainWindow>
#include <QMessageBox>
#include <QObject>
#include <spdlog/spdlog.h>

#include "mainwindow.h"
#include "nrsc.h"
#include "version.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , radio(new NRSC5)
{ 
    spdlog::info(PROGRAM " " VERSION);
    ui->setupUi(this);

    // Signal handling
    connect(ui->playButton, &QPushButton::pressed, this, &MainWindow::play);

    spdlog::info("Initialized main window");
}

void MainWindow::play()
{
    if(!playing) {

        spdlog::info("Playing...");

        // Parse the frequency
        freq = std::atof(ui->frequencyStr->text().toStdString().c_str());
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
        if(radio->init(-1) != 0) {
            QMessageBox::warning(this, "Error", "Failed to open SDR! Exiting...", QMessageBox::Ok);
            exit(-1);
        }

        // Adjust the UI
        ui->frequencyStr->setDisabled(true);
        ui->playButton->setText("Stop");
    }
    else {

        spdlog::info("Stopping...");

        // Stop the radio
        radio->stop();

        // Adjust the UI
        ui->frequencyStr->setDisabled(false);
        ui->playButton->setText("Play");
    }
    playing = !playing;
}