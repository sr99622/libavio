#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QSlider>

#include "avio.h"
#include "glwidget.h"
#include "progress.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void setPlayButton();

    QPushButton* btnPlay;
    QPushButton* btnStop;
    Progress* progress;
    GLWidget* glWidget;

protected:
    void closeEvent(QCloseEvent* event) override;

public slots:
    void onBtnPlayClicked();
    void onBtnStopClicked();
    void mediaPlayingStarted(qint64);
    void mediaPlayingStopped();
    void mediaProgress(float);

};


#endif // MAINWINDOW_H