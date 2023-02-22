/********************************************************************
* libavio/samples/include/mainwindow.h
*
* Copyright (c) 2023  Stephen Rhodes
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*********************************************************************/

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