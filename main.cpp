#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QCoreApplication::addLibraryPath("./");
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowFlags( Qt::Window | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint );
    w.show();
   // w.showFullScreen();
    return a.exec();
}
