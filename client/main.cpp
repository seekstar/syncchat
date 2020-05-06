#include "mainwindow.h"
#include <QApplication>

#include "mainmanager.h"

int main(int argc, char *argv[])
{
    //a = new QApplication(argc, argv);
    QApplication a(argc, argv);

    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<std::vector<uint8_t> >("std::vector<uint8_t>");
    qRegisterMetaType<LoginInfo>("LoginInfo");
    qRegisterMetaType<userid_t>("userid_t");
    qRegisterMetaType<msgcontent_t>("msgcontent_t");
    MainManager mainManager("127.0.0.1", "5188");
    //io_service.run();

    return a.exec();
}
