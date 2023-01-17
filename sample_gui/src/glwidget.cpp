#include <iostream>
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

void GLWidget::videoCallback(void* caller, const avio::Frame& frame)
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

void GLWidget::play()
{
    std::thread process_thread(start, this);
    process_thread.detach();
}

void GLWidget::start(void* parent)
{
    avio::Process process;

    avio::Reader reader("/home/stephen/Videos/news.mp4");
    reader.set_video_out("vpq_reader");
    reader.set_audio_out("apq_reader");

    //avio::Decoder videoDecoder(reader, AVMEDIA_TYPE_VIDEO, AV_HWDEVICE_TYPE_VAAPI);
    avio::Decoder videoDecoder(reader, AVMEDIA_TYPE_VIDEO, AV_HWDEVICE_TYPE_NONE);
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
    display.caller = parent;
    display.videoCallback = std::function(GLWidget::videoCallback);

    process.add_reader(reader);
    process.add_decoder(videoDecoder);
    process.add_filter(videoFilter);
    process.add_decoder(audioDecoder);
    process.add_display(display);

    process.run();

}

QSize GLWidget::sizeHint() const
{
    return QSize(640, 480);
}