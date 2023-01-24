#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QImage>
#include <QMutex>
#include <QPaintEvent>
#include "avio.h"

class GLWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    GLWidget();
    ~GLWidget();
    void play();
    void stop();
    void showStreamParameters(avio::Reader* reader);
    QSize sizeHint() const override;

    static void start(void* widget);
    static void renderCallback(void* caller, const avio::Frame& f);
    static void progressCallback(void* caller, float pct);
    static void infoCallback(void* caller, const std::string& msg);
    static void errorCallback(void* caller, const std::string& msg);

    QImage img;
    avio::Frame f;
    QMutex mutex;
    avio::Process* process = nullptr;

    bool disable_audio = false;
    bool disable_video = false;

    int volume = 100;
    bool mute = false;

signals:
    void mediaPlayingStarted(qint64);
    void mediaPlayingStopped();
    void mediaProgress(float);
    void criticalError(const QString&);
    void infoMessage(const QString&);

protected:
    void paintEvent(QPaintEvent* event) override;

public slots:
    void seek(float);

};

#endif // GLWIDGET_H