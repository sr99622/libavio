#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QMutex>
#include <QRect>
#include "avio.h"

class GLWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    GLWidget();
    ~GLWidget();
    void renderCallback(const avio::Frame& frame);
    QSize sizeHint() const override;
    QRect getImageRect(const QImage& img) const;

    avio::Frame f;
    QMutex mutex;

protected:
    void paintGL() override;

};

#endif // GLWIDGET_H