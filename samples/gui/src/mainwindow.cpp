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

    glWidget = new GLWidget();

    QWidget* pnlMain = new QWidget();
    QGridLayout* lytMain = new QGridLayout(pnlMain);
    lytMain->addWidget(btnPlay,   0, 1, 1, 1, Qt::AlignCenter);
    lytMain->addWidget(btnStop,   1, 1, 1, 1, Qt::AlignCenter);
    lytMain->addWidget(glWidget,  0, 0, 4, 1);
    lytMain->setColumnStretch(0, 10);
    setCentralWidget(pnlMain);
}

MainWindow::~MainWindow()
{

}

void MainWindow::onBtnPlayClicked()
{
    std::cout << "play" << std::endl;
    glWidget->play();
}

void MainWindow::onBtnStopClicked()
{
    std::cout << "stop" << std::endl;
    glWidget->stop();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    glWidget->stop();
    while (glWidget->process) {
        QThread::msleep(1);
    }
}