#ifndef SSLBASE_H_
#define SSLBASE_H_

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;
typedef boost::asio::ssl::context context_t;
typedef boost::asio::io_service io_service_t;

#endif //SSLBASE_H_
