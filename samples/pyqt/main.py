import sys
import avio
import numpy as np
from PyQt6.QtWidgets import QApplication, QMainWindow, QPushButton, \
QGridLayout, QWidget, QSlider
from PyQt6.QtCore import Qt, QSize
from PyQt6.QtGui import QPainter, QImage
from PyQt6.QtOpenGLWidgets import QOpenGLWidget

class AVWidget(QOpenGLWidget):
    def __init__(self):
        super().__init__()
        self.frame = avio.Frame()
        self.progress =  QSlider(Qt.Orientation.Horizontal)
        self.image = QImage()
        self.duration = 0

    def sizeHint(self):
        return QSize(640, 480)

    def renderCallback(self, f):
        ary = np.array(f, copy = False)
        h, w, d = ary.shape
        self.image = QImage(ary.data, w, h, d * w, QImage.Format.Format_RGB888)
        self.progress.setValue(int(100 * f.m_rts / self.duration))
        self.progress.update()
        self.update()

    def paintEvent(self, e):
        if (not self.image.isNull()):
            painter = QPainter(self)
            tmp = self.image.scaled(self.width(), self.height(), Qt.AspectRatioMode.KeepAspectRatio)
            dx = self.width() - tmp.width()
            dy = self.height() - tmp.height()
            painter.drawImage(int(dx/2), int(dy/2), tmp)

class MainWindow(QMainWindow):

    def __init__(self):
        super().__init__()

        self.setWindowTitle("avio")
        self.count = 0

        self.btnView = QPushButton("view")
        self.btnView.clicked.connect(self.btnViewClicked)

        self.avWidget = AVWidget()
        #self.progress = QSlider(Qt.Orientation.Horizontal)

        pnlMain = QWidget()
        lytMain = QGridLayout(pnlMain)
        lytMain.addWidget(self.avWidget,           0, 0, 3, 1)
        lytMain.addWidget(self.avWidget.progress,  3, 0, 1, 1)
        lytMain.addWidget(self.btnView,            0, 1, 1, 1, Qt.AlignmentFlag.AlignCenter)

        self.setCentralWidget(pnlMain)

    #def updateProgress(self, n):
    #    self.progress.setValue(int(n * 100))
    #    self.progress.update()
    #    QApplication.processEvents()

    def btnViewClicked(self):
        print("btnView clicked")

        self.process = avio.Process()

        self.reader = avio.Reader("/home/stephen/Videos/news.mp4")
        self.reader.set_video_out("vpq_reader")
        self.reader.set_audio_out("apq_reader")
        self.avWidget.duration = self.reader.duration()

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

        #def gen_p():
        #    return lambda n: self.updateProgress(n)
        #self.display.progressCallback = gen_p()

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