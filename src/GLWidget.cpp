
#include "GLWidget.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <iostream>
#include "avio.h"

#define VERTEX_ATTRIBUTE 0
#define TEXCOORD_ATTRIBUTE 1

namespace avio
{


GLWidget::GLWidget()
{
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(poll()));
    timer->start(poll_interval);
    connect(this, SIGNAL(doit()), this, SLOT(terminate()));
}

GLWidget::~GLWidget()
{
    timer->stop();
    delete timer;
    makeCurrent();
    vbo.destroy();
    delete texture;
    delete program;
    doneCurrent();
}

QSize GLWidget::sizeHint() const
{
    return QSize(640, 480);
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    const char *vsrc =
        "attribute highp vec4 vertex;\n"
        "attribute mediump vec4 texCoord;\n"
        "varying mediump vec4 texc;\n"
        "uniform mediump mat4 matrix;\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = matrix * vertex;\n"
        "    texc = texCoord;\n"
        "}\n";
    vshader->compileSourceCode(vsrc);

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    const char *fsrc =
        "uniform sampler2D texture;\n"
        "varying mediump vec4 texc;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragColor = texture2D(texture, texc.st);\n"
        "}\n";
    fshader->compileSourceCode(fsrc);

    program = new QOpenGLShaderProgram;
    program->addShader(vshader);
    program->addShader(fshader);
    program->bindAttributeLocation("vertex", VERTEX_ATTRIBUTE);
    program->bindAttributeLocation("texCoord", TEXCOORD_ATTRIBUTE);
    program->link();

    program->bind();
    program->setUniformValue("texture", 0);

    static const int coords[4][3] = {
        { +1, -1, -1 }, 
        { -1, -1, -1 }, 
        { -1, +1, -1 }, 
        { +1, +1, -1 }
    };

    QVector<GLfloat> vertData;
    for (int j = 0; j < 4; ++j) {
        vertData.append(coords[j][0]);
        vertData.append(coords[j][1]);
        vertData.append(coords[j][2]);

        vertData.append(j == 0 || j == 3);
        vertData.append(j == 0 || j == 1);
    }

    vbo.create();
    vbo.bind();
    vbo.allocate(vertData.constData(), vertData.count() * sizeof(GLfloat));
}

void GLWidget::paintGL()
{
    QColor clearColor(128, 0, 0);
    glClearColor(clearColor.redF(), clearColor.greenF(), clearColor.blueF(), clearColor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float B[4] = { 
        (-1.0f + pan_x) * zoom * factor * aspect,
        (+1.0f + pan_x) * zoom * factor * aspect,
        (+1.0f + pan_y) * zoom * factor / aspect,
        (-1.0f + pan_y) * zoom * factor / aspect 
    };

    QMatrix4x4 m;
    m.ortho(B[0], B[1], B[2], B[3], -4.0f, 15.0f);
    m.translate(0.0f, 0.0f, -10.0f);

    program->setUniformValue("matrix", m);

    program->enableAttributeArray(VERTEX_ATTRIBUTE);
    program->enableAttributeArray(TEXCOORD_ATTRIBUTE);
    program->setAttributeBuffer(VERTEX_ATTRIBUTE, GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
    program->setAttributeBuffer(TEXCOORD_ATTRIBUTE, GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));

    if (texture) {
        if (texture->width() != tex_width || texture->height() != tex_height) {
            delete texture;
            texture = nullptr;
        }
    }

    if (!texture) {
        texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        texture->setSize(tex_width, tex_height);
        texture->setFormat(QOpenGLTexture::RGB8_UNorm);
        texture->allocateStorage(QOpenGLTexture::RGB, QOpenGLTexture::UInt8);
        if (tex_width && tex_height)
            updateAspectRatio();
    }

    texture->bind();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

}

void GLWidget::updateAspectRatio()
{
    if (maintain_aspect_ratio) {
        if (texture) {
            float imageAspect = (float)texture->width() / (float)texture->height();
            float widgetAspect = (float)width() / (float)height();
            float ratio = imageAspect / widgetAspect;
            aspect = pow(ratio, -0.5);
            zoom = (ratio > 1.0 ? pow(ratio, 0.5) : pow(ratio, -0.5));
        }
    }
}

void GLWidget::resizeGL(int width, int height)
{
    if (maintain_aspect_ratio)
        updateAspectRatio();
}

void GLWidget::setZoomFactor(float arg)
{
    factor = arg;
    update();
}

void GLWidget::setPanX(int arg)
{
    pan_x = 1.0f - (float) arg / 50.0f;
    update();   
}

void GLWidget::setPanY(int arg)
{
    pan_y = 1.0f - (float) arg / 50.0f;
    update();   
}

void GLWidget::setFormat(QImage::Format arg)
{
    fmt = arg;
}

void GLWidget::setData(const uchar* data)
{
    QImage img(data, texture->width(), texture->height(), fmt);
    if (fmt != QImage::Format_RGB888)
        img = img.convertToFormat(QImage::Format_RGB888);
    img = img.mirrored();
    texture->setData(QOpenGLTexture::RGB, QOpenGLTexture::UInt8, (const void*)img.bits());
    update();
}

void GLWidget::poll()
{
    if (vfq_in) {
        try {
            while (vfq_in->size() > 0) {
                Frame f = vfq_in->pop();
                if (f.isValid())
                    setData(f.m_frame->data[0]);
            }
        }
        catch (const QueueClosedException& e) { }
    }
}

void GLWidget::play(const char* uri)
{
    process_thread = new std::thread(buildProcess, this, uri);
}

void GLWidget::pause()
{
    process->key_event(SDLK_SPACE);
}

void GLWidget::stop()
{
    if (process) {
        process->key_event(SDLK_ESCAPE);
        process_thread->join();
        process = nullptr;
    }
}

void GLWidget::buildProcess(void * parent, const char* uri)
{
    GLWidget* widget = (GLWidget*)parent;
    widget->process = new avio::Process();

    avio::Reader reader(uri);
    widget->tex_width = reader.width();
    widget->tex_height = reader.height();
    reader.set_video_out("vpq_reader");

    avio::Decoder videoDecoder(reader, AVMEDIA_TYPE_VIDEO);
    videoDecoder.set_video_in(reader.video_out());
    videoDecoder.set_video_out("vfq_decoder");

    avio::Filter videoFilter(videoDecoder, "format=rgb24");
    videoFilter.set_video_in(videoDecoder.video_out());
    videoFilter.set_video_out("vfq_filter");

    avio::Display display(reader);
    display.set_video_in(videoFilter.video_out());
    display.set_video_out("vfq_display");
    display.enable_sdl_video = false;
    widget->set_video_in(display.video_out());

    avio::Decoder* audioDecoder;
    if (reader.has_audio()) {
        reader.set_audio_out("apq_reader");
        audioDecoder = new avio::Decoder(reader, AVMEDIA_TYPE_AUDIO);
        audioDecoder->set_audio_in(reader.audio_out());
        audioDecoder->set_audio_out("afq_decoder");
        display.set_audio_in(audioDecoder->audio_out());
        widget->process->add_decoder(*audioDecoder);
    }

    widget->process->add_reader(reader);
    widget->process->add_decoder(videoDecoder);
    widget->process->add_filter(videoFilter);
    widget->process->add_display(display);
    widget->process->add_widget(widget);

    widget->process->run();

    std::cout << "glwidget process done" << std::endl;
    if (audioDecoder)
        delete audioDecoder;

    widget->emit doit();
    std::cout << "glwidget process done 2" << std::endl;

}

void GLWidget::terminate()
{
    std::cout << "terminate" << std::endl;
    using namespace std::chrono_literals;
    std::cout << "Hello waiter\n" << std::flush;
    auto start = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(200ms);

    if (process_thread->joinable()) {
        process_thread->join();
        //delete process_thread;
    }
}

}