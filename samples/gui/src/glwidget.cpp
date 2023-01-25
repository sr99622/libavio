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

void GLWidget::infoCallback(void* caller, const std::string& msg)
{
    GLWidget* glWidget = (GLWidget*)caller;
    glWidget->emit infoMessage(msg.c_str());
}

void GLWidget::errorCallback(void* caller, const std::string& msg)
{
    GLWidget* glWidget = (GLWidget*)caller;
    glWidget->emit criticalError(msg.c_str());
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
    std::cout << "stop finish" << std::endl;
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

    try {
        avio::Process process;
        process.infoCaller = widget;
        process.infoCallback = std::function(GLWidget::infoCallback);
        process.errorCaller = widget;
        process.errorCallback = std::function(GLWidget::errorCallback);
        glWidget->process = &process;

        avio::Reader reader("/home/stephen/Videos/five.webm");
        reader.process = &process;
        reader.showStreamParameters();

        avio::Display display(reader);
        display.renderCaller = widget;
        display.renderCallback = std::function(GLWidget::renderCallback);
        display.progressCaller = widget;
        display.progressCallback = std::function(GLWidget::progressCallback);
        display.volume = (float)glWidget->volume / 100.0f;
        display.mute = glWidget->mute;

        avio::Decoder* videoDecoder = nullptr;
        avio::Filter* videoFilter = nullptr;
        if (reader.has_video() && !glWidget->disable_video) {
            reader.set_video_out("vpq_reader");
            //reader.show_video_pkts = true;
            videoDecoder = new avio::Decoder(reader, AVMEDIA_TYPE_VIDEO, AV_HWDEVICE_TYPE_NONE);
            videoDecoder->process = &process;
            videoDecoder->set_video_in(reader.video_out());
            videoDecoder->set_video_out("vfq_decoder");
            process.add_decoder(*videoDecoder);
            videoFilter = new avio::Filter(*videoDecoder, "format=rgb24");
            videoFilter->process = &process;
            videoFilter->set_video_in(videoDecoder->video_out());
            videoFilter->set_video_out("vfq_filter");
            process.add_filter(*videoFilter);
            display.set_video_in(videoFilter->video_out());
        }

        avio::Decoder* audioDecoder = nullptr;
        if (reader.has_audio() && !glWidget->disable_audio) {
            reader.set_audio_out("apq_reader");
            audioDecoder = new avio::Decoder(reader, AVMEDIA_TYPE_AUDIO);
            audioDecoder->process = &process;
            audioDecoder->set_audio_in(reader.audio_out());
            audioDecoder->set_audio_out("afq_decoder");
            display.set_audio_in(audioDecoder->audio_out());
            process.add_decoder(*audioDecoder);
        }

        process.add_reader(reader);
        process.add_display(display);

        glWidget->emit mediaPlayingStarted(reader.duration());
        process.run();
        glWidget->process = nullptr;
        glWidget->emit mediaPlayingStopped();

        if (videoDecoder) delete videoDecoder;
        if (videoFilter) delete videoFilter;
        if (audioDecoder) delete audioDecoder;
    }
    catch (const avio::Exception& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        glWidget->stop();
        glWidget->process = nullptr;
        glWidget->emit criticalError(e.what());
    }
}

QSize GLWidget::sizeHint() const
{
    return QSize(640, 480);
}