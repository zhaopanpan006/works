#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{

//    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
//    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
//    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);//控制图片缩放质量

    QApplication a(argc, argv);
    MainWindow w;

    w.show();
    w.showMaximized();

    return a.exec();
}
