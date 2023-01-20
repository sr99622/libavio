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

    QImage img;
    avio::Frame f;
    QMutex mutex;
    avio::Process* process = nullptr;

protected:
    void paintEvent(QPaintEvent* event) override;

};

#endif // GLWIDGET_H