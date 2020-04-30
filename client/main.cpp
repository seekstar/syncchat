#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>

#include "sslclient.h"

int main(int argc, char *argv[])
{
    //a = new QApplication(argc, argv);
    QApplication a(argc, argv);

    sslconn();

    return a.exec();
}
