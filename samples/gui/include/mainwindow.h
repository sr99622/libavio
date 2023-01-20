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

    QPushButton* btnPlay;
    QPushButton* btnStop;
    GLWidget* glWidget;

protected:
    void closeEvent(QCloseEvent* event) override;

public slots:
    void onBtnPlayClicked();
    void onBtnStopClicked();

};


#endif // MAINWINDOW_H