#include "sslmanager.h"

#include <QDebug>

#include <iostream>
#include <boost/bind.hpp>

SslManager::SslManager(const char *ip, const char *port, QObject *parent)
    : QObject(parent),
      work(new boost::asio::io_service::work(io_service)),
      run_io_service([&] {
          std::cerr << "The id of the new thread is " << std::this_thread::get_id() << std::endl;
          qDebug() << "io_service going to run in another thread!";
          io_service.run();
          qDebug() << "io_service stopped";
      }),
      resolver(io_service),
      query(ip, port),
      iterator(resolver.resolve(query)),
      context(NULL),
      //socket_(io_service, context),
      socket_(NULL),
      busy(false)
{
    run_io_service.detach();
    std::cerr << "The id of the main thread is " << std::this_thread::get_id() << std::endl;
}

SslManager::~SslManager() {
    work.reset();
}

ssl_socket *
SslManager::socket() {
    return socket_;
}

#define HANDLE_ERROR        \
    if (error) {            \
        emit sigErr(std::string("Error: ") + __PRETTY_FUNCTION__ + "\n" + error.message());\
        busy = false;       \
        delete context;     \
        context = NULL;     \
        delete socket_;     \
        socket_ = NULL;     \
        return;             \
    }

void SslManager::handle_handshake(const boost::system::error_code& error)
{
    HANDLE_ERROR;
    qDebug() << "handshake done";
    emit sigDone(socket_);
    busy = false;
}

void SslManager::handle_connect(const boost::system::error_code& error)
{
    std::cerr << "After async call, the thread id is " << std::this_thread::get_id() << std::endl;
    qDebug() << "Enter handle_connect";
    HANDLE_ERROR;
    socket_->async_handshake(boost::asio::ssl::stream_base::client,
                             boost::bind(&SslManager::handle_handshake, this,
                                         boost::asio::placeholders::error));
}

bool SslManager::verify_certificate(bool preverified,
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
    qDebug() << "Verifying" << subject_name;

    return preverified;
}

void SslManager::sslconn() {
    if (busy) {
        qDebug() << "SslManager busy";
        return;
    }
    busy = true;
    qDebug() << "sslconn";
    context = new context_t(boost::asio::ssl::context::sslv23);
    context->load_verify_file("ca.pem");
    socket_ = new ssl_socket(io_service, *context);
    socket_->set_verify_mode(boost::asio::ssl::verify_peer);
    socket_->set_verify_callback(boost::bind(&SslManager::verify_certificate, this, _1, _2));
    qDebug() << "before async_connect";
    std::cerr << "The thread id now is " << std::this_thread::get_id() << std::endl;
    boost::asio::async_connect(socket_->lowest_layer(), iterator,
                               boost::bind(&SslManager::handle_connect, this, boost::asio::placeholders::error));
}
