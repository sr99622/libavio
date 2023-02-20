#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QSlider>

#include "avio.h"
#include "glwidget.h"
#include "progress.h"

class Label : public QLabel
{
    Q_OBJECT

    QSize sizeHint() const{
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

    QPushButton* btnPlay;
    QPushButton* btnStop;
    QPushButton* btnRecord;
    QPushButton* btnMute;
    QSlider* sldVolume;
    Progress* progress;
    GLWidget* glWidget;
    avio::Player* player = nullptr;
    bool mute = false;
    const char* uri;

protected:
    void closeEvent(QCloseEvent* event) override;

signals:
    void uiUpdate();
    void showError(const QString&);

public slots:
    void onBtnPlayClicked();
    void onBtnStopClicked();
    void onBtnRecordClicked();
    void onBtnMuteClicked();
    void onSldVolumeChanged(int);
    void seek(float);
    void mediaPlayingStarted(qint64);
    void mediaPlayingStopped();
    void mediaProgress(float);
    void criticalError(const std::string&);
    void infoMessage(const std::string&);
    void updateUI();
    void onShowError(const QString&);

};


#endif // MAINWINDOW_H