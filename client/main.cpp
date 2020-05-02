#include "mainwindow.h"
#include <QApplication>

#include "mainmanager.h"

#include <QTimer>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

int main(int argc, char *argv[])
{
    //a = new QApplication(argc, argv);
    QApplication a(argc, argv);

    boost::asio::io_service io_service;

    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query(argv[1], argv[2]);
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

    boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
    ctx.load_verify_file("ca.pem");

    client c(io_service, ctx, iterator);
    //io_service.run();

    return a.exec();
}
