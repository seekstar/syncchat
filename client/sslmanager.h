#ifndef SSLMANAGER_H
#define SSLMANAGER_H

#include <QObject>

class SslManager : public QObject
{
    Q_OBJECT
public:
    explicit SslManager(boost::asio::io_service& io_service,
                        boost::asio::ssl::context& context,
                        QObject *parent = 0);

signals:

public slots:

private:
    ssl_socket socket_;
};

#endif // SSLMANAGER_H
