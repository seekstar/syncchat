#ifndef SSLMANAGER_H
#define SSLMANAGER_H

#include <QObject>
#include <QTimer>

#include <thread>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "sslbase.h"

class SslManager : public QObject
{
    Q_OBJECT
public:
    explicit SslManager(const char *ip, const char *port, QObject *parent = 0);
    ~SslManager();
    ssl_socket *socket();

signals:
    void sigErr(std::string);
    void sigDone(ssl_socket *sock);

public slots:
    void sslconn();

private:
    bool verify_certificate(bool preverified,
                            boost::asio::ssl::verify_context& ctx);
    void handle_connect(const boost::system::error_code& error);
    void handle_handshake(const boost::system::error_code& error);

    QTimer timer;
    boost::asio::io_service io_service;
    std::unique_ptr<boost::asio::io_service::work> work;
    std::thread run_io_service;
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::ip::tcp::resolver::query  query;
    boost::asio::ip::tcp::resolver::iterator iterator;
    typedef boost::asio::ssl::context context_t;
    context_t *context;
    ssl_socket *socket_;
    bool busy;
};

#endif // SSLMANAGER_H
