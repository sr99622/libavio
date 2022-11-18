
#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QTimer>
#include <QMouseEvent>
#include <iostream>
#include "Queue.h"
#include "Frame.h"

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram);
QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)

namespace avio
{

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    GLWidget();
    ~GLWidget();

    void setZoomFactor(float);
    void setPanX(int);
    void setPanY(int);
    void setData(const uchar *);
    void setFormat(QImage::Format);
    void updateAspectRatio();

    QSize sizeHint() const override;

    std::string vfq_in_name;
    std::string vfq_out_name;

    Queue<Frame>* vfq_in = nullptr;
    Queue<Frame>* vfq_out = nullptr;

    std::string video_in() const { return std::string(vfq_in_name); }
    std::string video_out() const { return std::string(vfq_out_name); }
    void set_video_in(const std::string& name) { vfq_in_name = std::string(name); }
    void set_video_out(const std::string& name) { vfq_out_name = std::string(name); }

    int poll_interval = 1;
    int tex_width = 0;
    int tex_height = 0;
    bool maintain_aspect_ratio = true;

public slots:
    void poll();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

private:
    QOpenGLTexture *texture = nullptr;
    QOpenGLShaderProgram *program = nullptr;
    QOpenGLBuffer vbo;

    float zoom   = 1.0f;
    float factor = 1.0f;
    float aspect = 1.0f;
    float pan_x  = 1.0f;
    float pan_y  = 1.0f;

    QImage::Format fmt = QImage::Format_RGB888;

    QTimer *timer;

};

}

#endif
