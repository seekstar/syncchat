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
    qRegisterMetaType<CppContent>("msgcontent_t");
    qRegisterMetaType<grpid_t>("grpid_t");
    qRegisterMetaType<momentid_t>("momentid_t");
    qRegisterMetaType<commentid_t>("commentid_t");
    qRegisterMetaType<CppContent>("CppContent");
    MainManager mainManager("127.0.0.1", "5188");
    //io_service.run();

    return a.exec();
}
