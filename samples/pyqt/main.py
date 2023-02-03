import sys
import avio
from PyQt6.QtWidgets import QApplication, QMainWindow, QPushButton, \
QGridLayout, QWidget, QLabel, QSlider
from PyQt6.QtCore import Qt, QSize, QMutex, QRect
from PyQt6.QtGui import QPainter, QBrush, QColor, QImage
from PyQt6.QtOpenGLWidgets import QOpenGLWidget
#import cb
import numpy as np

class AVWidget(QOpenGLWidget):
    def __init__(self):
        super().__init__()
        self.mutex = QMutex()
        self.frame = avio.Frame()
        self.image = QImage()

    def sizeHint(self):
        return QSize(640, 480)

    def renderCallback(self, f):
        self.mutex.lock()
        self.frame = f
        self.img = np.array(f, copy = True)
        print(self.img.shape)
        h, w, _ = self.img.shape
        self.image = QImage(self.img.data, w, h, 3 * w, QImage.Format.Format_RGB888)
        #self.image = QImage(self.img.data, self.img.shape[1], self.img.shape[0], QImage.Format.Format_RGB888)
        print("do-wop frame: ", f.width(), f.height(), f.stride())
        #self.image = QImage(self.frame.data(), self.frame.width(), self.frame.height(), QImage.Format.Format_RGB888)
        self.mutex.unlock()
        print("renderCallback", f.m_rts)
        self.update()

    def paintEvent(self, e):
        print("paintEvent")
        self.mutex.lock()
        #self.image = QImage("/home/stephen/Pictures/test.png")
        print("width: ", self.image.width(), " height: ", self.image.height())
        if (not self.image.isNull()):
            print("imge valid", self.image.width(), self.image.height())
            painter = QPainter(self)
            tmp = self.image.scaled(self.width(), self.height(), Qt.AspectRatioMode.KeepAspectRatio)
            dx = self.width() - tmp.width()
            dy = self.height() - tmp.height()
            painter.drawImage(int(dx/2), int(dy/2), tmp)
        self.mutex.unlock()

'''

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
        #print("test", self.frame.m_rts)
        #print("width", self.frame.width())
        #print("height", self.frame.height())
        #painter = QPainter(self)
        #brush = QBrush()
        #brush.setColor(QColor('red'))
        #brush.setStyle(Qt.BrushStyle.SolidPattern)
        #rect = QRect(0, 0, painter.device().width(), painter.device().height())
        #painter.fillRect(rect, brush)        

std::function<void(const avio::Frame& frame)> renderCallback = [&](const avio::Frame& frame)
{
    if (!frame.isValid()) {
        glWidget->emit infoMessage("render callback recvd invalid Frame");
        return;
    }
    glWidget->mutex.lock();
    glWidget->f = frame;
    glWidget->img = QImage(glWidget->f.m_frame->data[0], glWidget->f.m_frame->width,
                            glWidget->f.m_frame->height, QImage::Format_RGB888);
    glWidget->mutex.unlock();
    glWidget->update();
};

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

'''

class MainWindow(QMainWindow):

    def __init__(self):
        super().__init__()

        self.setWindowTitle("My App")
        self.count = 0

        self.btnView = QPushButton("view")
        self.btnView.clicked.connect(self.btnViewClicked)

        self.btnTest = QPushButton("test")
        self.btnTest.clicked.connect(self.btnTestClicked)

        self.lblCount = QLabel(str(self.count))

        self.avWidget = AVWidget()
        self.progress = QSlider(Qt.Orientation.Horizontal)

        pnlMain = QWidget()
        lytMain = QGridLayout(pnlMain)
        lytMain.addWidget(self.avWidget,  0, 0, 3, 1)
        lytMain.addWidget(self.progress,  3, 0, 1, 1)
        lytMain.addWidget(self.btnView,   0, 1, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytMain.addWidget(self.btnTest,   1, 1, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytMain.addWidget(self.lblCount,  2, 1, 1, 1)

        self.setCentralWidget(pnlMain)

        #def gen_f():
        #    return lambda n: self.updateLabel(n)
        #self.w = cb.Widget(10, gen_f())


    def btnTestClicked(self):
        print("btnTest clicked")
        self.w.twink()

    def updateLabel(self, n):
        self.lblCount.setText(str(n))
        self.lblCount.update()

    def updateProgress(self, n):
        print(n)
        self.progress.setValue(int(n * 100))
        self.progress.update()
        QApplication.processEvents()

    def btnViewClicked(self):
        print("btnView clicked")

        self.process = avio.Process()

        self.reader = avio.Reader("/home/stephen/Videos/news.mp4")
        self.reader.set_video_out("vpq_reader")
        self.reader.set_audio_out("apq_reader")

        self.videoDecoder = avio.Decoder(self.reader, avio.AVMEDIA_TYPE_VIDEO)
        self.videoDecoder.set_video_in(self.reader.video_out())
        self.videoDecoder.set_video_out("vfq_decoder")

        #videoFilter = avio.Filter(videoDecoder, "scale=1280x720,format=rgb24")
        self.videoFilter = avio.Filter(self.videoDecoder, "format=rgb24")
        self.videoFilter.set_video_in(self.videoDecoder.video_out())
        self.videoFilter.set_video_out("vfq_filter")

        self.audioDecoder = avio.Decoder(self.reader, avio.AVMEDIA_TYPE_AUDIO)
        self.audioDecoder.set_audio_in(self.reader.audio_out())
        self.audioDecoder.set_audio_out("afq_decoder")

        self.display = avio.Display(self.reader)
        self.display.set_video_in(self.videoFilter.video_out())
        self.display.set_audio_in(self.audioDecoder.audio_out())
        def gen_p():
            return lambda n: self.updateProgress(n)
        self.display.progressCallback = gen_p()


        def gen_f():
            return lambda f: self.avWidget.renderCallback(f)
        self.display.renderCallback = gen_f()

        self.process.add_reader(self.reader)
        self.process.add_decoder(self.videoDecoder)
        self.process.add_filter(self.videoFilter)
        self.process.add_decoder(self.audioDecoder)
        self.process.add_display(self.display)

        self.process.start()


if __name__ == '__main__':

    app = QApplication(sys.argv)

    window = MainWindow()
    window.show()

    app.exec()