
#include "GLWidget.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QTimer>
#include <iostream>
#include "avio.h"

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1

namespace avio
{


GLWidget::GLWidget(int width, int height) : width(width), height(height)
{
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(poll()));
    timer->start(poll_interval);
}

GLWidget::~GLWidget()
{
    makeCurrent();
    vbo.destroy();
    delete texture;
    delete program;
    doneCurrent();
}

QSize GLWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize GLWidget::sizeHint() const
{
    return QSize(400, 400);
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
    program->bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
    program->bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
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
        // vertex position
        vertData.append(coords[j][0]);
        vertData.append(coords[j][1]);
        vertData.append(coords[j][2]);
        // texture coordinate
        vertData.append(j == 0 || j == 3);
        vertData.append(j == 0 || j == 1);
    }

    vbo.create();
    vbo.bind();
    vbo.allocate(vertData.constData(), vertData.count() * sizeof(GLfloat));

    texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    texture->setSize(width, height);
    texture->setFormat(QOpenGLTexture::RGB8_UNorm);
    texture->allocateStorage(QOpenGLTexture::RGB, QOpenGLTexture::UInt8);

}

void GLWidget::paintGL()
{
    QColor clearColor(128, 0, 0);
    glClearColor(clearColor.redF(), clearColor.greenF(), clearColor.blueF(), clearColor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float B[4] = { 
        (-1.0f + pan_x) * zoom * aspect,
        (+1.0f + pan_x) * zoom * aspect,
        (+1.0f + pan_y) * zoom / aspect,
        (-1.0f + pan_y) * zoom / aspect 
    };

    QMatrix4x4 m;
    m.ortho(B[0], B[1], B[2], B[3], -4.0f, 15.0f);
    m.translate(0.0f, 0.0f, -10.0f);

    program->setUniformValue("matrix", m);

    program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    program->setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
    program->setAttributeBuffer(PROGRAM_TEXCOORD_ATTRIBUTE, GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));

    texture->bind();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

}

void GLWidget::resizeGL(int width, int height)
{
    int side = qMin(width, height);
    std::cout << "resizeGL: " << width << " x " << height << std::endl;
    //glViewport((width - side) / 2, (height - side) / 2, side, side);
}

void GLWidget::setZoom(int arg) 
{
    zoom = (float) arg / 50.0f;
    update();
}

void GLWidget::setAspect(int arg)
{
    aspect = (float) arg / 50.0f;
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
    texture->setData(QOpenGLTexture::RGB, QOpenGLTexture::UInt8, img.bits());
    update();
}

void GLWidget::poll()
{
    if (vfq_in) {
        try {
            while (vfq_in->size() > 0) {
                Frame f = vfq_in->pop();
                setData(f.m_frame->data[0]);
            }
        }
        catch (const QueueClosedException& e) { }
    }
}

}
