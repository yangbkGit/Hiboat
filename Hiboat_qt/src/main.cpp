#include "mainwindow.h"
#include <QApplication>

#include <iostream>
using namespace std;


extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    cout << "Hello FFmpeg!" <<endl;
    av_register_all();
    unsigned version = avcodec_version();
    cout << "version is:" <<version <<endl;

    MainWindow w;
    w.show();
    return a.exec();
}
