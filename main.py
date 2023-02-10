import sys
import build.avio as avio
import numpy as np
import cv2
import torch
import torchvision
import torchvision.transforms as transforms
from PyQt6.QtWidgets import QApplication, QMainWindow, QPushButton, \
QGridLayout, QWidget, QSlider, QLabel, QMessageBox
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
        self.image = QImage()

    def sizeHint(self):
        return QSize(640, 480)

    def renderCallback(self, f):
        ary = np.array(f, copy = False)
        h, w, d = ary.shape
        self.image = QImage(ary.data, w, h, d * w, QImage.Format.Format_RGB888)
        self.update()

    #def paintGL(self):
    #def paintEvent(self, e):
    #    if (not self.image.isNull()):
    #        painter = QPainter(self)
    #        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
    #        tmp = self.image.scaled(self.width(), self.height(), Qt.AspectRatioMode.KeepAspectRatio)
    #        dx = self.width() - tmp.width()
    #        dy = self.height() - tmp.height()
    #        painter.drawImage(int(dx/2), int(dy/2), tmp)

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

    def cbFrame(self, f):
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

    def showInfo(self, str):
        print("SHOW INFO", str)

    def showError(self, str):
        print("SHOW ERROR", str)
        self.avWidget.setText(str)

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
        self.player.uri = "C:/Users/sr996/Videos/odessa.mp4"
        self.player.hw_device_type = avio.AV_HWDEVICE_TYPE_CUDA
        self.player.video_filter = "format=bgr24,fps=5"
        self.player.audio_filter = "anull"
        self.player.cbFrame = lambda f: self.cbFrame(f)
        self.player.cbProgress = lambda n: self.updateProgress(n)
        self.player.hWnd = self.avWidget.winId()
        self.player.cbWidth = lambda : self.avWidget.width()
        self.player.cbHeight = lambda : self.avWidget.height()
        self.player.cbInfo = lambda s: self.showInfo(s)
        self.player.cbError = lambda s: self.showError(s)

        self.player.start()


if __name__ == '__main__':

    app = QApplication(sys.argv)

    window = MainWindow()
    window.show()

    app.exec()