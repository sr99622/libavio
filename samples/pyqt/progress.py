from PyQt6.QtWidgets import QSlider, QLabel, QWidget, QGridLayout
from PyQt6.QtCore import Qt, QPoint
from PyQt6.QtGui import QPainter

class Slider(QSlider):
    def __init__(self, o, P):
        super().__init__(o)
        self.P = P
        if P.showPosition:
            self.setMouseTracking(True)

    def leaveEvent(self, e):
        self.P.updatePosition(-1, 0)

    def mousePressEvent(self, e):
        pct = e.position().x() / self.width()
        self.P.w.player.seek(pct)

    def mouseMoveEvent(self, e):
        if (self.P.w.playing):
            x = e.position().x()
            pct = x / self.width()
            self.P.updatePosition(pct, x)

class Position(QLabel):
    def __init__(self):
        super().__init__()
        self.pos = 0

    def setText(self, s, n):
        self.pos = n
        super().setText(s)

    def paintEvent(self, e):
        painter = QPainter(self)
        rect = self.fontMetrics().boundingRect(self.text())
        x = min(self.width() - rect.width(), self.pos)
        painter.drawText(QPoint(int(x), self.height()), self.text())

class Progress(QWidget):
    def __init__(self, w):
        super().__init__()
        self.w = w
        self.duration = 0
        self.showPosition = True

        self.sldProgress = Slider(Qt.Orientation.Horizontal, self)
        self.sldProgress.setMaximum(1000)
        self.lblProgress = QLabel("0:00")
        self.setLabelWidth(self.lblProgress)
        self.lblDuration = QLabel("0:00")
        self.setLabelWidth(self.lblDuration)
        if self.showPosition:
            self.lblPosition = Position()

        lytProgress = QGridLayout(self)
        if self.showPosition:
            lytProgress.addWidget(self.lblPosition,  0, 1, 1, 1)
        lytProgress.addWidget(self.lblProgress,      1, 0, 1, 1)
        lytProgress.addWidget(self.sldProgress,      1, 1, 1, 1)
        lytProgress.addWidget(self.lblDuration,      1, 2, 1, 1)
        lytProgress.setContentsMargins(0, 0, 0, 0)
        lytProgress.setColumnStretch(1, 10)

    def setLabelWidth(self, l):
        l.setFixedWidth(l.fontMetrics().boundingRect("00:00:00").width())
        l.setAlignment(Qt.AlignmentFlag.AlignCenter)

    def updateDuration(self, n):
        self.duration = n
        self.lblDuration.setText(self.timestring(n))

    def updateProgress(self, f):
        self.sldProgress.setValue(int(f * 1000))
        self.lblProgress.setText(self.timestring(int(self.duration * f)))

    def updatePosition(self, f, n):
        if self.showPosition:
            if f >= 0:
                position = int(f * self.duration)
                self.lblPosition.setText(self.timestring(position), n)
            else:
                self.lblPosition.setText("", 0)

    def timestring(self, n):
        time_interval = int(n / 1000)
        hours = int(time_interval / 3600)
        minutes = int ((time_interval - (hours * 3600)) / 60)
        seconds = int ((time_interval - (hours * 3600) - (minutes * 60)))
        if hours > 0:
            buf = "%02d:%02d:%02d" % (hours, minutes, seconds)
        else:
            buf = "%d:%02d" % (minutes, seconds)
        return buf

