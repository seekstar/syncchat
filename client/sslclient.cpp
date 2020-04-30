#include "sslclient.h"
#include <QMessageBox>
#include <QEventLoop>

#include <iostream>
#include <boost/bind.hpp>

#include "winlogin.h"

ssl_socket *socket_;

void handle_error(const boost::system::error_code& error, const char *title) {
    QMessageBox::critical(NULL, title, QString(error.message().c_str()));
    delete socket_;
    //a->quit();
    //QEventLoop::quit();
    exit(0);
}

void handle_handshake(const boost::system::error_code& error)
{
    if (error) {
        handle_error(error, "handshake error");
        return;
    }
    std::cerr << "handshake done\n";
    WinLogin *winLogin = new WinLogin;
    winLogin->setAttribute(Qt::WA_DeleteOnClose);
    winLogin->show();

    /*mainWindow = new MainWindow;
    const char *dataSource = "wechatclient";
    std::string errorInfo = mainWindow->login(dataSource);
    if (errorInfo != "") {
        QMessageBox::critical(NULL, "Login error", QString(errorInfo.c_str()) +
                              "\nPlease make sure that there is an odbc data source named " + dataSource + '!');
        mainWindow->deleteLater();
        //return 0;
    }*/
    //mainWindow->show();
}

void handle_connect(const boost::system::error_code& error)
{
    if (error) {
        handle_error(error, "Connect failed");
        return;
    }
    socket_->async_handshake(boost::asio::ssl::stream_base::client, handle_handshake);
}

bool verify_certificate(bool preverified,
                        boost::asio::ssl::verify_context& ctx)
{
    // The verify callback can be used to check whether the certificate that is
    // being presented is valid for the peer. For example, RFC 2818 describes
    // the steps involved in doing this for HTTPS. Consult the OpenSSL
    // documentation for more details. Note that the callback is called once
    // for each certificate in the certificate chain, starting from the root
    // certificate authority.

    // In this example we will simply print the certificate's subject name.
    char subject_name[256];
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    std::cerr << "Verifying " << subject_name << "\n";

    return preverified;
}

void sslconn() {
    boost::asio::io_service io_service;

    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query("127.0.0.1", "5188");
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

    boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
    //system("pwd");
    ctx.load_verify_file("ca.pem");

    socket_ = new ssl_socket(io_service, ctx);
    socket_->set_verify_mode(boost::asio::ssl::verify_peer);
    socket_->set_verify_callback(verify_certificate);

    boost::asio::async_connect(socket_->lowest_layer(), iterator,
                               boost::bind(handle_connect, boost::asio::placeholders::error));

    io_service.run();
}
