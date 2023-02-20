#include <iostream>
#include <filesystem>
#include <QGridLayout>
#include <QMessageBox>
#include "mainwindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("Main Window");
    
    btnPlay = new QPushButton("Play");
    connect(btnPlay, SIGNAL(clicked()), this, SLOT(onBtnPlayClicked()));

    btnStop = new QPushButton("Stop");
    connect(btnStop, SIGNAL(clicked()), this, SLOT(onBtnStopClicked()));

    btnRecord = new QPushButton("Record");
    connect(btnRecord, SIGNAL(clicked()), this, SLOT(onBtnRecordClicked()));

    btnMute = new QPushButton("Mute");
    connect(btnMute, SIGNAL(clicked()), this, SLOT(onBtnMuteClicked()));

    sldVolume = new QSlider();
    sldVolume->setValue(80);
    sldVolume->setTracking(true);
    connect(sldVolume, SIGNAL(valueChanged(int)), this, SLOT(onSldVolumeChanged(int)));

    QWidget* pnlControl = new QWidget();
    QGridLayout* lytControl = new QGridLayout(pnlControl);
    lytControl->addWidget(btnPlay,      0, 0, 1, 1, Qt::AlignCenter);
    lytControl->addWidget(btnStop,      1, 0, 1, 1, Qt::AlignCenter);
    lytControl->addWidget(btnRecord,    2, 0, 1, 1, Qt::AlignCenter);
    lytControl->addWidget(sldVolume,    3, 0, 1, 1, Qt::AlignCenter);
    lytControl->addWidget(btnMute,      4, 0, 1, 1, Qt::AlignCenter);
    lytControl->addWidget(new QLabel(), 5, 0, 1, 1);
    lytControl->setRowStretch(5, 10);

    progress = new Progress(this);

    glWidget = new GLWidget();

    QWidget* pnlMain = new QWidget();
    QGridLayout* lytMain = new QGridLayout(pnlMain);
    lytMain->addWidget(glWidget,    0, 0, 4, 1);
    lytMain->addWidget(pnlControl,  0, 1, 5, 1);
    lytMain->addWidget(progress,    4, 0, 1, 1);
    lytMain->setColumnStretch(0, 10);
    lytMain->setRowStretch(0, 10);
    setCentralWidget(pnlMain);

    connect(this, SIGNAL(uiUpdate()), this, SLOT(updateUI()));
    connect(this, SIGNAL(showError(const QString&)), this, SLOT(onShowError(const QString&)));
    connect(progress, SIGNAL(seek(float)), this, SLOT(seek(float)));
}

MainWindow::~MainWindow()
{

}

void MainWindow::setPlayButton()
{
    if (player) {
        if (player->isPaused()) {
            btnPlay->setText("Play");
        }
        else {
            btnPlay->setText("Pause");
        }
    }
    else {
        btnPlay->setText("Play");
    }
}

void MainWindow::setRecordButton()
{
    if (player) {
        if (player->isPiping()) {
            btnRecord->setText("Recording");
        }
        else {
            btnRecord->setText("Record");
        }
    }
    else {
        btnRecord->setText("Record");
    }
}

void MainWindow::onBtnPlayClicked()
{
    if (player) {
        player->togglePaused();
    }
    else {
        player = new avio::Player();
        player->width = [&]() { return glWidget->width(); };
        player->height = [&]() { return glWidget->height(); };
        player->uri = uri;
        player->video_filter = "format=rgb24";
        player->renderCallback = [&](const avio::Frame& frame) { glWidget->renderCallback(frame); };
        player->progressCallback = [&](float arg) { progress->setProgress(arg); };
        player->cbMediaPlayingStarted = [&](int64_t duration) { mediaPlayingStarted(duration); };
        player->cbMediaPlayingStopped = [&]() { mediaPlayingStopped(); };
        player->errorCallback = [&](const std::string& msg) { criticalError(msg); };
        player->infoCallback = [&](const std::string& msg) { infoMessage(msg); };
        player->setMute(mute);
        player->setVolume(sldVolume->value());
        player->start();

    }
    setPlayButton();
}

void MainWindow::onBtnStopClicked()
{
    if (player) {
        player->running = false;
    }
}

void MainWindow::onBtnRecordClicked()
{
    if (player) {
        std::string ext = std::filesystem::path(player->uri).extension().string();
        std::stringstream str;
        str << "output" << ext;
        player->togglePiping(str.str());
    }
    setRecordButton();
}

void MainWindow::onBtnMuteClicked()
{
    mute = !mute;
    if (player)
        player->setMute(mute);
    if (mute)
        btnMute->setText("UnMute");
    else
        btnMute->setText("Mute");
}

void MainWindow::onSldVolumeChanged(int arg)
{
    if (player) player->setVolume(arg);
}

void MainWindow::updateUI()
{
    setPlayButton();
    setRecordButton();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (player) {
        player->running = false;
    }
    while (player) std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void MainWindow::seek(float arg)
{
    if (player) player->seek(arg);
}

void MainWindow::mediaPlayingStarted(qint64 duration)
{
    progress->setDuration(duration);
    emit uiUpdate();
}

void MainWindow::mediaPlayingStopped()
{
    progress->setProgress(0);
    emit uiUpdate();
    delete player;
    player = nullptr;
    glWidget->f.invalidate();
    glWidget->update();
}

void MainWindow::mediaProgress(float arg)
{
    progress->setProgress(arg);
}

void MainWindow::onShowError(const QString& msg)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Critical Error");
    msgBox.setText(msg);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();
    setPlayButton();
    setRecordButton();
}

void MainWindow::criticalError(const std::string& msg)
{
    emit showError(msg.c_str());
}

void MainWindow::infoMessage(const std::string& msg)
{
    std::cout << msg << std::endl;
}