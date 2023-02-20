#include <iostream>
#include <QPainter>
#include <QImage>

#include "glwidget.h"

GLWidget::GLWidget()
{

}

GLWidget::~GLWidget()
{

}

QRect GLWidget::getImageRect(const QImage& img) const
{
    float ratio = std::min((float)width() / (float)img.width(), (float)height() / (float)img.height());
    float w = (float)img.width() * ratio;
    float h = (float)img.height() * ratio;
    float x = ((float)width() - w) / 2.0f;
    float y = ((float)height() - h) / 2.0f;
    return QRect((int)x, (int)y, (int)w, (int)h);
}

void GLWidget::paintGL()
{
    if (f.isValid()) {
        mutex.lock();
        QPainter painter;
        painter.begin(this);
        QImage img = QImage(f.m_frame->data[0], f.m_frame->width, f.m_frame->height, QImage::Format_RGB888);
        painter.drawImage(getImageRect(img), img);
        painter.end();
        mutex.unlock();
    }
}

void GLWidget::renderCallback(const avio::Frame& frame)
{
    if (!frame.isValid()) {
        std:: cout << "render callback recvd invalid Frame" << std::endl;
        return;
    }
    mutex.lock();
    f = frame;
    mutex.unlock();
    update();
}

QSize GLWidget::sizeHint() const
{
    return QSize(640, 480);
}