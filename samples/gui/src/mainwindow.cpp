#include <iostream>
#include <QGridLayout>
#include "mainwindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("Main Window");
    
    btnTest = new QPushButton("Test");
    connect(btnTest, SIGNAL(clicked()), this, SLOT(onBtnTestClicked()));

    glWidget = new GLWidget();

    QWidget* pnlMain = new QWidget();
    QGridLayout* lytMain = new QGridLayout(pnlMain);
    lytMain->addWidget(btnTest,   0, 1, 1, 1, Qt::AlignCenter);
    lytMain->addWidget(glWidget,  0, 0, 1, 1);
    lytMain->setColumnStretch(0, 10);
    setCentralWidget(pnlMain);
}

MainWindow::~MainWindow()
{

}

void MainWindow::onBtnTestClicked()
{
    std::cout << "test" << std::endl;
    glWidget->play();
}