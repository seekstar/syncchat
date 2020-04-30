#ifndef SSLBASE_H_
#define SSLBASE_H_

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

#endif //SSLBASE_H_