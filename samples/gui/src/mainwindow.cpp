#include <iostream>
#include <QGridLayout>
#include <QThread>
#include "mainwindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("Main Window");
    
    btnPlay = new QPushButton("Play");
    connect(btnPlay, SIGNAL(clicked()), this, SLOT(onBtnPlayClicked()));

    btnStop = new QPushButton("Stop");
    connect(btnStop, SIGNAL(clicked()), this, SLOT(onBtnStopClicked()));

    progress = new Progress(this);

    glWidget = new GLWidget();
    connect(glWidget, SIGNAL(mediaPlayingStarted(qint64)), this, SLOT(mediaPlayingStarted(qint64)));
    connect(glWidget, SIGNAL(mediaPlayingStopped()), this, SLOT(mediaPlayingStopped()));
    connect(glWidget, SIGNAL(mediaProgress(float)), this, SLOT(mediaProgress(float)));
    connect(progress, SIGNAL(seek(float)), glWidget, SLOT(seek(float)));

    QWidget* pnlMain = new QWidget();
    QGridLayout* lytMain = new QGridLayout(pnlMain);
    lytMain->addWidget(btnPlay,     0, 1, 1, 1, Qt::AlignCenter);
    lytMain->addWidget(btnStop,     1, 1, 1, 1, Qt::AlignCenter);
    lytMain->addWidget(glWidget,    0, 0, 4, 1);
    lytMain->addWidget(progress,    4, 0, 1, 1);
    lytMain->setColumnStretch(0, 10);
    setCentralWidget(pnlMain);
}

MainWindow::~MainWindow()
{

}

void MainWindow::setPlayButton()
{
    if (glWidget->process) {
        if (glWidget->process->isPaused()) {
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

void MainWindow::onBtnPlayClicked()
{
    std::cout << "play" << std::endl;
    if (glWidget->process) {
        glWidget->process->togglePaused();
    }
    else {
        glWidget->play();
    }
    setPlayButton();
}

void MainWindow::onBtnStopClicked()
{
    std::cout << "stop" << std::endl;
    glWidget->stop();
    setPlayButton();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    glWidget->stop();
    while (glWidget->process) {
        QThread::msleep(1);
    }
}

void MainWindow::mediaPlayingStarted(qint64 duration)
{
    std::cout << "MainWindow::mediaPlayingStarted" << std::endl;
    progress->setDuration(duration);
    setPlayButton();
}

void MainWindow::mediaPlayingStopped()
{
    std::cout << "MainWindow::mediaPlayingStopped" << std::endl;
    progress->setProgress(0);
    setPlayButton();
}

void MainWindow::mediaProgress(float arg)
{
    //sldProgress->setValue(std::round(sldProgress->maximum() * arg));
    progress->setProgress(arg);
}