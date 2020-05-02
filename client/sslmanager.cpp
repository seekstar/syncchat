#include "sslmanager.h"

SslManager::SslManager(boost::asio::io_service& io_service,
                       boost::asio::ssl::context& context,
                       QObject *parent)
    : QObject(parent),
      socket(io_service, context)
{

}
