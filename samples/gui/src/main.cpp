#include <QApplication>
#include "mainwindow.h"

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "Usage: sample-cmd filename" << std::endl;
        return 1;
    }

    std::cout << "playing file: " << argv[1] << std::endl;

    QApplication a(argc, argv);
    MainWindow w;
    w.uri = argv[1];
    w.show();
    return a.exec();
}