import sys
import avio
from PyQt6.QtWidgets import QApplication, QMainWindow, QPushButton, \
QGridLayout, QWidget
from PyQt6.QtCore import Qt, QSize
from PyQt6.QtOpenGLWidgets import QOpenGLWidget

class AVWidget(QOpenGLWidget):
    def __init__(self):
        super().__init__()

    def sizeHint(self):
        return QSize(640, 480)



class MainWindow(QMainWindow):

    def __init__(self):
        super().__init__()

        self.setWindowTitle("My App")

        btnTest = QPushButton("test")
        btnTest.clicked.connect(self.btnTestClicked)

        avWidget = AVWidget()

        pnlMain = QWidget()
        lytMain = QGridLayout(pnlMain)
        lytMain.addWidget(avWidget,  0, 0, 1, 1)
        lytMain.addWidget(btnTest,   0, 1, 1, 1, Qt.AlignmentFlag.AlignCenter)

        self.setCentralWidget(pnlMain)

    def btnTestClicked(self):
        print("clicked")

        process = avio.Process()

        reader = avio.Reader("/home/stephen/Videos/short.mp4")
        reader.set_video_out("vpq_reader")
        reader.set_audio_out("apq_reader")

        videoDecoder = avio.Decoder(reader, avio.AVMEDIA_TYPE_VIDEO)
        videoDecoder.set_video_in(reader.video_out())
        videoDecoder.set_video_out("vfq_decoder")

        #videoFilter = avio.Filter(videoDecoder, "scale=1280x720,format=rgb24")
        videoFilter = avio.Filter(videoDecoder, "scale=1280x720")
        videoFilter.set_video_in(videoDecoder.video_out())
        videoFilter.set_video_out("vfq_filter")

        audioDecoder = avio.Decoder(reader, avio.AVMEDIA_TYPE_AUDIO)
        audioDecoder.set_audio_in(reader.audio_out())
        audioDecoder.set_audio_out("afq_decoder")

        display = avio.Display(reader)
        display.set_video_in(videoFilter.video_out())
        display.set_audio_in(audioDecoder.audio_out())

        process.add_reader(reader)
        process.add_decoder(videoDecoder)
        process.add_filter(videoFilter)
        process.add_decoder(audioDecoder)
        process.add_display(display)

        process.run()

if __name__ == '__main__':

    app = QApplication(sys.argv)

    window = MainWindow()
    window.show()

    app.exec()