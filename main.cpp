#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setWindowIcon(QIcon(":/icons/appicon.png")); // from resources


    MainWindow w;
    w.show();
    return a.exec();
}
