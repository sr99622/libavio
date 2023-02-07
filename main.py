import sys
import build.avio as avio
import numpy as np
import cv2
import torch
import torchvision
import torchvision.transforms as transforms
from PyQt6.QtWidgets import QApplication, QMainWindow, QPushButton, \
QGridLayout, QWidget, QSlider, QLabel
from PyQt6.QtCore import Qt, QSize
from PyQt6.QtGui import QPainter, QImage, QGuiApplication
from PyQt6.QtOpenGLWidgets import QOpenGLWidget

transform = transforms.Compose([
    transforms.ToTensor(),
])


class AVWidget(QLabel):
    def __init__(self, p):
        super().__init__()
        self.frame = avio.Frame()
        #self.progress =  QSlider(Qt.Orientation.Horizontal)
        self.image = QImage()
        self.duration = 0
        self.player = p

    def sizeHint(self):
        return QSize(640, 480)

    def resizeEvent(self, e):
        width = e.size().width()
        height = e.size().height()
        print("spontaneous: ", e.spontaneous())
        print("resizeEvent w: ", width, " h: ", height)
        if not e.spontaneous():
            self.player.width = e.size().width()
            self.player.height = e.size().height()

    def renderCallback(self, f):
        ary = np.array(f, copy = False)
        h, w, d = ary.shape
        self.image = QImage(ary.data, w, h, d * w, QImage.Format.Format_RGB888)
        self.update()

    #def paintGL(self):
    def paintEvent(self, e):
        if (not self.image.isNull()):
            painter = QPainter(self)
            painter.setRenderHint(QPainter.RenderHint.Antialiasing)
            tmp = self.image.scaled(self.width(), self.height(), Qt.AspectRatioMode.KeepAspectRatio)
            dx = self.width() - tmp.width()
            dy = self.height() - tmp.height()
            painter.drawImage(int(dx/2), int(dy/2), tmp)

class MainWindow(QMainWindow):

    def __init__(self):
        super().__init__()
        self.setWindowTitle("avio")
        self.count = 0
        self.player = avio.Player()

        self.btnPlay = QPushButton("play")
        self.btnPlay.clicked.connect(self.btnPlayClicked)

        self.btnPause = QPushButton("pause")
        self.btnPause.clicked.connect(self.btnPauseClicked)

        self.btnStop = QPushButton("stop")
        self.btnStop.clicked.connect(self.btnStopClicked)

        self.avWidget = AVWidget(self.player)
        self.progress = QSlider(Qt.Orientation.Horizontal)

        pnlMain = QWidget()
        lytMain = QGridLayout(pnlMain)
        lytMain.addWidget(self.avWidget,           0, 0, 6, 1)
        lytMain.addWidget(self.progress,           6, 0, 1, 1)
        lytMain.addWidget(self.btnPlay,            0, 1, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytMain.addWidget(self.btnPause,           1, 1, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytMain.addWidget(self.btnStop,            2, 1, 1, 1, Qt.AlignmentFlag.AlignCenter)

        self.setCentralWidget(pnlMain)

        self.min_size = 800
        self.threshold = 0.35
        self.model = torchvision.models.detection.retinanet_resnet50_fpn(pretrained=True,  min_size=self.min_size)
        self.device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
        self.model.eval().to(self.device)

    def closeEvent(self, e):
        print(e)
        self.player.running = False
        from time import sleep
        sleep(0.5)

    def pythonCallback(self, f):
        print("rts:", f.m_rts)
        img = np.array(f, copy = False)

        tensor = transform(img).to(self.device)
        tensor = tensor.unsqueeze(0)

        with torch.no_grad():
            outputs = self.model(tensor)

        scores = outputs[0]['scores'].detach().cpu().numpy()
        labels = outputs[0]['labels'].detach().cpu().numpy()
        boxes = outputs[0]['boxes'].detach().cpu().numpy()
        labels = labels[np.array(scores) >= self.threshold]
        boxes = boxes[np.array(scores) >= self.threshold].astype(np.int32)
        boxes = boxes[np.array(labels) == 1]

        for box in boxes:
            cv2.rectangle(img, (box[0], box[1]), (box[2], box[3]), (255, 255, 255), 1)

        return f

    def updateProgress(self, n):
        self.progress.setValue(int(n * 100))
        self.progress.update()

    def btnPauseClicked(self):
        print("btnPauseClicked")
        self.player.togglePaused()

    def btnStopClicked(self):
        print("btnStopClicked")
        self.player.running = False


    def btnPlayClicked(self):
        print("btnPlay clicked")
        if self.player.running:
            return

        self.player = avio.Player()
        self.avWidget.player = self.player

        self.reader = avio.Reader("C:/Users/sr996/Videos/odessa.mp4")
        self.reader.set_video_out("vpq_reader")
        self.reader.set_audio_out("apq_reader")
        self.avWidget.duration = self.reader.duration()

        self.videoDecoder = avio.Decoder(self.reader, avio.AVMEDIA_TYPE_VIDEO, avio.AV_HWDEVICE_TYPE_CUDA)
        #self.videoDecoder = avio.Decoder(self.reader, avio.AVMEDIA_TYPE_VIDEO, avio.AV_HWDEVICE_TYPE_VDPAU)
        self.videoDecoder.set_video_in(self.reader.video_out())
        self.videoDecoder.set_video_out("vfq_decoder")

        #videoFilter = avio.Filter(videoDecoder, "scale=1280x720,format=rgb24")
        self.videoFilter = avio.Filter(self.videoDecoder, "format=bgr24, fps=5")
        self.videoFilter.set_video_in(self.videoDecoder.video_out())
        self.videoFilter.set_video_out("vfq_filter")

        self.audioDecoder = avio.Decoder(self.reader, avio.AVMEDIA_TYPE_AUDIO)
        self.audioDecoder.set_audio_in(self.reader.audio_out())
        self.audioDecoder.set_audio_out("afq_decoder")

        self.display = avio.Display(self.reader)
        self.display.set_video_in(self.videoFilter.video_out())
        self.display.set_audio_in(self.audioDecoder.audio_out())

        self.display.progressCallback = lambda n: self.updateProgress(n)
        print(QGuiApplication.platformName())
        self.display.hWnd = self.avWidget.winId()
        print("widget width: ", self.avWidget.width(), " height: ", self.avWidget.height())
        self.player.width = self.avWidget.width()
        self.player.height = self.avWidget.height()

        #if QGuiApplication.platformName() == "windows":
        #    self.display.hWnd = self.avWidget.winId()
        #else:
        #    self.display.renderCallback = lambda f: self.avWidget.renderCallback(f)

        self.display.pythonCallback = lambda f: self.pythonCallback(f)

        self.player.add_reader(self.reader)
        self.player.add_decoder(self.videoDecoder)
        self.player.add_filter(self.videoFilter)
        self.player.add_decoder(self.audioDecoder)
        self.player.add_display(self.display)

        self.player.start()


if __name__ == '__main__':

    app = QApplication(sys.argv)

    window = MainWindow()
    window.show()

    app.exec()