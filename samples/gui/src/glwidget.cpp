#include <iostream>
#include <SDL.h>
#include <QPainter>
#include "glwidget.h"

GLWidget::GLWidget()
{

}

GLWidget::~GLWidget()
{

}

void GLWidget::paintEvent(QPaintEvent* event)
{
    QOpenGLWidget::paintEvent(event);
    QPainter painter;
    painter.begin(this);
    if (!img.isNull()) {
        mutex.lock();
        QImage tmp = img.scaled(width(), height(), Qt::KeepAspectRatio);
        int dx = width() - tmp.width();
        int dy = height() - tmp.height();
        painter.drawImage(dx>>1, dy>>1, tmp);
        mutex.unlock();
    }
    painter.end();
}

void GLWidget::renderCallback(void* caller, const avio::Frame& frame)
{
    if (!frame.isValid()) {
        std::cout << "call recvd invalid frame" << std::endl;
        return;
    }

    GLWidget* g = (GLWidget*)caller;
    g->mutex.lock();
    g->f = frame;
    g->img = QImage(g->f.m_frame->data[0], g->f.m_frame->width,
                        g->f.m_frame->height, QImage::Format_RGB888);
    g->mutex.unlock();
    g->update();
        
}

void GLWidget::progressCallback(void* caller, float pct)
{
    GLWidget* g = (GLWidget*)caller;
    g->emit mediaProgress(pct);
}

void GLWidget::stop()
{
    std::cout << "GLWidget::stop" << std::endl;
    if (process) {
        process->running = false;
        if (process->isPaused()) {
            std::cout << "process paused" << std::endl;
            SDL_Event event;
            event.type = SDL_QUIT;
            SDL_PushEvent(&event);
        }
    }
}

void GLWidget::play()
{
    std::thread process_thread(start, this);
    process_thread.detach();
}

void GLWidget::seek(float arg)
{
    std::cout << "GLWidget::seek: " << arg << std::endl;
    if (process) process->seek(arg);
}

void GLWidget::start(void* widget)
{
    GLWidget* glWidget = (GLWidget*)widget;
    avio::Process process;
    glWidget->process = &process;

    avio::Reader reader("/home/stephen/Videos/five.webm");
    reader.set_video_out("vpq_reader");
    reader.set_audio_out("apq_reader");
    reader.show_video_pkts = true;

    avio::Decoder videoDecoder(reader, AVMEDIA_TYPE_VIDEO, AV_HWDEVICE_TYPE_VAAPI);
    //avio::Decoder videoDecoder(reader, AVMEDIA_TYPE_VIDEO, AV_HWDEVICE_TYPE_NONE);
    videoDecoder.set_video_in(reader.video_out());
    videoDecoder.set_video_out("vfq_decoder");

    avio::Filter videoFilter(videoDecoder, "format=rgb24");
    videoFilter.set_video_in(videoDecoder.video_out());
    videoFilter.set_video_out("vfq_filter");
    //videoFilter.show_frames = true;

    avio::Decoder audioDecoder(reader, AVMEDIA_TYPE_AUDIO);
    audioDecoder.set_audio_in(reader.audio_out());
    audioDecoder.set_audio_out("afq_decoder");

    avio::Display display(reader);
    display.set_video_in(videoFilter.video_out());
    //display.set_video_in(videoDecoder.video_out());
    display.set_audio_in(audioDecoder.audio_out());
    display.renderCaller = widget;
    display.renderCallback = std::function(GLWidget::renderCallback);
    display.progressCaller = widget;
    display.progressCallback = std::function(GLWidget::progressCallback);

    process.add_reader(reader);
    process.add_decoder(videoDecoder);
    process.add_filter(videoFilter);
    process.add_decoder(audioDecoder);
    process.add_display(display);

    glWidget->emit mediaPlayingStarted(reader.duration());
    process.run();
    glWidget->process = nullptr;
    glWidget->emit mediaPlayingStopped();
}

QSize GLWidget::sizeHint() const
{
    return QSize(640, 480);
}