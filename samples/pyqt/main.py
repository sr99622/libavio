import os
import sys
import numpy as np
import cv2
from time import sleep
from PyQt6.QtWidgets import QApplication, QMainWindow, QPushButton, \
QGridLayout, QWidget, QSlider, QLabel, QMessageBox
from PyQt6.QtCore import Qt, pyqtSignal, QObject
from glwidget import GLWidget
from progress import Progress

sys.path.append("../../build")
import avio


class Signals(QObject):
    error = pyqtSignal(str)
    progress = pyqtSignal(float)
    starting = pyqtSignal(int)
    stopped = pyqtSignal()

class MainWindow(QMainWindow):

    def __init__(self, filename):
        super().__init__()
        self.setWindowTitle("avio")
        self.showProgress = True

        if self.showProgress:
            self.progress = Progress(self)

        self.signals = Signals()
        self.signals.error.connect(self.showErrorDialog)
        self.signals.starting.connect(self.playerStarted)
        self.signals.stopped.connect(self.playerStopped)
        if self.showProgress:
            self.signals.progress.connect(self.progress.updateProgress)

        self.playing = False
        self.closing = False
        self.mute = False
        self.duration = 0
        self.uri = filename
        self.player = avio.Player()

        self.btnPlay = QPushButton("Play")
        self.btnPlay.clicked.connect(self.btnPlayClicked)
        self.btnStop = QPushButton("Stop")
        self.btnStop.clicked.connect(self.btnStopClicked)
        self.btnRecord = QPushButton("Record")
        self.btnRecord.clicked.connect(self.btnRecordClicked)
        self.btnMute = QPushButton("Mute")
        self.btnMute.clicked.connect(self.btnMuteClicked)
        self.sldVolume = QSlider()
        self.sldVolume.setValue(80)
        self.sldVolume.setTracking(True)
        self.sldVolume.valueChanged.connect(self.sldVolumeChanged)
        pnlControl = QWidget()
        lytControl = QGridLayout(pnlControl)
        lytControl.addWidget(self.btnPlay,    0, 0, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytControl.addWidget(self.btnStop,    1, 0, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytControl.addWidget(self.btnRecord,  2, 0, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytControl.addWidget(self.sldVolume,  3, 0, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytControl.addWidget(self.btnMute,    4, 0, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytControl.addWidget(QLabel(),        5, 0, 1, 1)
        lytControl.setRowStretch(5, 10)

        self.glWidget = GLWidget()
        
        pnlMain = QWidget()
        lytMain = QGridLayout(pnlMain)
        lytMain.addWidget(self.glWidget,   0, 0, 1, 1)
        if self.showProgress:
            lytMain.addWidget(self.progress,   1, 0, 1, 1)
        lytMain.addWidget(pnlControl,      0, 1, 2, 1)
        lytMain.setColumnStretch(0, 10)
        lytMain.setRowStretch(0, 10)

        self.setCentralWidget(pnlMain)

    def pythonCallback(self, F):
        img = np.array(F, copy = False)
        cv2.rectangle(img, (500, 600), (900, 800), (0, 255, 0), 5)
        return F

    def btnMuteClicked(self):
        self.mute = not self.mute
        self.player.setMute(self.mute)
        if self.mute:
            self.btnMute.setText("UnMute")
        else:
            self.btnMute.setText("Mute")

    def btnRecordClicked(self):
        if self.playing:
            _, file_ext = os.path.splitext(self.player.uri)
            print(file_ext)
            filename = "output" + file_ext
            self.player.togglePiping(filename)
        self.setRecordButton()

    def sldVolumeChanged(self, value):
        self.player.setVolume(value)

    def showErrorDialog(self, s):
        msgBox = QMessageBox(self)
        msgBox.setWindowTitle("Error")
        msgBox.setText(s)
        msgBox.setStandardButtons(QMessageBox.StandardButton.Ok)
        msgBox.setIcon(QMessageBox.Icon.Critical)
        msgBox.exec()
        self.setPlayButton()
        self.setRecordButton()

    def closeEvent(self, e):
        self.closing = True
        self.player.running = False
        while self.playing:
            sleep(0.001)

    def btnStopClicked(self):
        self.player.running = False

    def playerStarted(self, n):
        self.setPlayButton()
        if self.showProgress:
            self.progress.updateDuration(n)

    def playerStopped(self):
        if not self.closing:
            self.glWidget.clear()
            self.setPlayButton()
            self.setRecordButton()
            if self.showProgress:
                self.progress.updateProgress(0)
                self.progress.updateDuration(0)

    def mediaPlayingStopped(self):
        self.playing = False
        self.signals.stopped.emit()

    def mediaPlayingStarted(self, n):
        self.signals.starting.emit(n)

    def errorCallback(self, s):
        self.signals.error.emit(s)

    def progressCallback(self, f):
        if self.showProgress:
            self.signals.progress.emit(f)

    def infoCallback(self, s):
        print(s)

    def setPlayButton(self):
        if self.playing:
            if self.player.isPaused():
                self.btnPlay.setText("Play")
            else:
                self.btnPlay.setText("Pause")
        else:
            self.btnPlay.setText("Play")

    def setRecordButton(self):
        if self.playing:
            if self.player.isPiping():
                self.btnRecord.setText("Recording")
            else:
                self.btnRecord.setText("Record")
        else:
            self.btnRecord.setText("Record")

    def btnPlayClicked(self):
        if self.playing:
            self.player.togglePaused()
            self.setPlayButton()
        else:
            self.playing = True
            self.player.uri = self.uri
            self.player.width = lambda : self.glWidget.width()
            self.player.height = lambda : self.glWidget.height()
            self.player.progressCallback = lambda f : self.progressCallback(f)
            self.player.video_filter = "format=rgb24"
            self.player.renderCallback = lambda F : self.glWidget.renderCallback(F)
            self.player.pythonCallback = lambda F : self.pythonCallback(F)
            self.player.cbMediaPlayingStarted = lambda n : self.mediaPlayingStarted(n)
            self.player.cbMediaPlayingStopped = lambda : self.mediaPlayingStopped()
            self.player.errorCallback = lambda s : self.errorCallback(s)
            self.player.infoCallback = lambda s : self.infoCallback(s)
            self.player.setVolume(self.sldVolume.value())
            self.player.setMute(self.mute)
            #self.player.disable_video = True
            #self.player.hw_device_type = avio.AV_HWDEVICE_TYPE_QSV
            self.player.start()

if __name__ == '__main__':

    if len(sys.argv) < 2:
        print("please specify media filename in command line")
    else:
        print(sys.argv[1])
        app = QApplication(sys.argv)
        window = MainWindow(sys.argv[1])
        window.show()

        app.exec()