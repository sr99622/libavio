#include <iostream>
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

    progress = new Progress(this);
    connect(progress, SIGNAL(seek(float)), this, SLOT(seek(float)));

    label = new Label();

    QWidget* pnlMain = new QWidget();
    QGridLayout* lytMain = new QGridLayout(pnlMain);
    lytMain->addWidget(btnPlay,     0, 1, 1, 1, Qt::AlignCenter);
    lytMain->addWidget(btnStop,     1, 1, 1, 1, Qt::AlignCenter);
    lytMain->addWidget(btnRecord,   2, 1, 1, 1, Qt::AlignCenter);
    lytMain->addWidget(label,       0, 0, 4, 1);
    lytMain->addWidget(progress,    4, 0, 1, 1);
    lytMain->setColumnStretch(0, 10);
    setCentralWidget(pnlMain);
}

MainWindow::~MainWindow()
{

}

void MainWindow::setPlayButton()
{
    if (player) {
        if (player->isPaused())
            btnPlay->setText("Play");
        else
            btnPlay->setText("Pause");
    }
    else {
        btnPlay->setText("Play");
    }
}

void MainWindow::setRecordButton()
{
    if (player) {
        if (player->isPiping())
            btnRecord->setText("=*=*=");
        else
            btnRecord->setText("Record");
    }
    else {
        btnRecord->setText("Record");
    }
}

void MainWindow::onBtnPlayClicked()
{
    std::cout << "play" << std::endl;
    if (player) {
        player->togglePaused();
    }
    else {
        player = new avio::Player();
        player->uri = "C:/Users/sr996/Videos/news.mp4";
        player->hWnd = label->winId();
        player->cbWidth = [&]() { return label->width(); };
        player->cbHeight = [&]() { return label->height(); };
        player->cbInfo = [&](const std::string& msg) { infoMessage(msg); };
        player->cbError = [&](const std::string& msg) { criticalError(msg); };
        player->cbProgress = [&](float pct) { progress->setProgress(pct); };
        player->cbMediaPlayingStarted = [&](int64_t duration) { mediaPlayingStarted(duration); };
        player->cbMediaPlayingStopped = [&]() { mediaPlayingStopped(); };
        player->start();
    }
    setPlayButton();
}

void MainWindow::onBtnStopClicked()
{
    std::cout << "stop" << std::endl;
    if (player) {
        player->running = false;
        if (player->isPaused()) player->togglePaused();
    }
}

void MainWindow::onBtnRecordClicked()
{
    std::cout << "record" << std::endl;
    if (player) player->toggle_pipe_out("test.mp4");
    setRecordButton();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (player) {
        player->running = false;
        while (player) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void MainWindow::mediaPlayingStarted(qint64 duration)
{
    std::cout << "mediaPlayingStarted" << std::endl;
    progress->setDuration(duration);
}

void MainWindow::mediaPlayingStopped()
{
    std::cout << "mediaPlayingStopped" << std::endl;
    progress->setProgress(0);
    player = nullptr;
    setPlayButton();
    setRecordButton();
}

void MainWindow::seek(float pct)
{
    std::cout << "seek: " << pct << std::endl;
    if (player) player->seek(pct);
}

void MainWindow::criticalError(const std::string& msg)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Critical Error");
    msgBox.setText(msg.c_str());
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();
    setPlayButton();
    setRecordButton();
}

void MainWindow::infoMessage(const std::string& msg)
{
    std::cout << "message from mainwindow" << std::endl;
    std::cout << msg << std::endl;
}