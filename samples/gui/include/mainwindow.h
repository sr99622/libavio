#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QSlider>

#include "avio.h"
#include "progress.h"

 class Label : public QLabel
 {
    Q_OBJECT

    QSize Label::sizeHint() const
    {
        return QSize(640, 480);
    }
 };

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void setPlayButton();
    void setRecordButton();
    void mediaPlayingStarted(qint64);
    void mediaPlayingStopped();
    void criticalError(const std::string&);
    void infoMessage(const std::string&);

    QPushButton* btnPlay;
    QPushButton* btnStop;
    QPushButton* btnRecord;
    Progress* progress;
    Label* label;
    avio::Player* player = nullptr;

protected:
    void closeEvent(QCloseEvent* event) override;

public slots:
    void onBtnPlayClicked();
    void onBtnStopClicked();
    void onBtnRecordClicked();
    void seek(float);

};


#endif // MAINWINDOW_H