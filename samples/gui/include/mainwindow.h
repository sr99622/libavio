#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>

#include "avio.h"
#include "glwidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    QPushButton* btnTest;
    GLWidget* glWidget;

public slots:
    void onBtnTestClicked();

};


#endif // MAINWINDOW_H