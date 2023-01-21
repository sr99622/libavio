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
    QSize sizeHint() const override;

    static void start(void* widget);
    static void renderCallback(void* caller, const avio::Frame& f);
    static void progressCallback(void* caller, float pct);

    QImage img;
    avio::Frame f;
    QMutex mutex;
    avio::Process* process = nullptr;

signals:
    void mediaPlayingStarted(qint64);
    void mediaPlayingStopped();
    void mediaProgress(float);

protected:
    void paintEvent(QPaintEvent* event) override;

public slots:
    void seek(float);

};

#endif // GLWIDGET_H