#include <cstdlib>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "session.h"

class server
{
public:
  server(boost::asio::io_service& io_service, unsigned short port)
    : io_service_(io_service),
      acceptor_(io_service,
          boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      context_(boost::asio::ssl::context::sslv23)
  {
    context_.set_options(
        boost::asio::ssl::context::default_workarounds
        | boost::asio::ssl::context::no_sslv2
        | boost::asio::ssl::context::single_dh_use);
    //context_.set_password_callback(boost::bind(&server::get_password, this));
    context_.use_certificate_chain_file("server.pem");
    context_.use_private_key_file("server.pem", boost::asio::ssl::context::pem);
    context_.use_tmp_dh_file("dh2048.pem");

    start_accept();
  }

  /*std::string get_password() const
  {
    return "test";
  }*/

  void start_accept()
  {
    session* new_session = new session(io_service_, context_);
    acceptor_.async_accept(new_session->socket(),
        boost::bind(&server::handle_accept, this, new_session,
          boost::asio::placeholders::error));
  }

  void handle_accept(session* new_session,
      const boost::system::error_code& error)
  {
    if (!error)
    {
      new_session->start();
    }
    else
    {
      delete new_session;
    }

    start_accept();
  }

private:
  boost::asio::io_service& io_service_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ssl::context context_;
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: server <port>\n";
      return 1;
    }

    boost::asio::io_service io_service;

    using namespace std; // For atoi.
    server s(io_service, atoi(argv[1]));

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
