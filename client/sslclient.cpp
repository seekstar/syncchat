#include "sslclient.h"
#include <QMessageBox>
#include <QEventLoop>
#include <QDebug>

#include <iostream>
#include <boost/bind.hpp>

#include "winlogin.h"
#include "types.h"

transaction_t last_tsid = 0;

void handle_error(const boost::system::error_code& error, const char *title) {
    QMessageBox::critical(NULL, title, QString(error.message().c_str()));
    delete socket_;
    dialogReconnect->show();
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
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

    boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
    //system("pwd");
    ctx.load_verify_file("ca.pem");

    socket_->set_verify_mode(boost::asio::ssl::verify_peer);
    socket_->set_verify_callback(verify_certificate);

    boost::asio::async_connect(socket_->lowest_layer(), iterator,
                               boost::bind(handle_connect, boost::asio::placeholders::error));

    if (io_service.stopped()) {
        qDebug() << "run io_service";
        io_service.run();
        qDebug() << "io_service stopped";
    }
}

void reconnect() {
    delete socket_;
    sslconn();
}
