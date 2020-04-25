#include <cstdlib>
#include <iostream>
#include <unordered_map>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "escape.h"
#include "types.h"
#include "srvtypes.h"
#include "dbop.h"

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

class session;
std::unordered_map<userid_t, session*> user_session;

class session {
public:
    session(boost::asio::io_service& io_service,
        boost::asio::ssl::context& context)
      : socket_(io_service, context)
    {
        //to = NULL;
    }

    ssl_socket::lowest_layer_type& socket() {
        return socket_.lowest_layer();
    }

    void start() {
        socket_.async_handshake(boost::asio::ssl::stream_base::server,
            boost::bind(&session::handshake_done, this,
            boost::asio::placeholders::error));
    }
    void handshake_done(const boost::system::error_code& error) {
        if (error) {
            delete this;
            return;
        }
        user_session[userid] = this;
        listen_request(boost::system::errc::make_error_code(boost::system::errc::success));
    }
    void listen_request(const boost::system::error_code& error) {
        if (error) {
            reset();
            return;
        }
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_, sizeof(enum C2S)),
            boost::bind(&session::handle_request, this, boost::asio::placeholders::error));
    }
    void handle_request(const boost::system::error_code& error) {
        if (error) {
            reset();
            return;
        }
        switch (*(enum C2S*)buf_) {
        case C2S::MSG:
            boost::asio::async_read(socket_,
                boost::asio::buffer(buf_, sizeof(struct MsgSendHeader)),
                boost::bind(&session::handle_msg, this, boost::asio::placeholders::error));
            break;
        }
    }
    void handle_msg(const boost::system::error_code& error) {
        if (error) {
            reset();
            return;
        }
        struct MsgSendHeader *msgSendHeader = (struct MsgSendHeader*)(buf_);
        if (msgSendHeader->len > MAX_CONTENT_LEN) {
            *(enum S2C*)buf_ = S2C::FAIL;
            *(enum S2CFAIL*)(buf_ + sizeof(enum S2C)) = S2CFAIL::MSG_TOO_LONG;
            boost::asio::async_write(socket_,
                boost::asio::buffer(buf_, sizeof(S2C) + sizeof(S2CFAIL)),
                boost::bind(&session::listen_request, this, boost::asio::placeholders::error));
        }
        touser_ = msgSendHeader->to;
        msgRecvHeader.from = userid;
        msgRecvHeader.reply = msgSendHeader->reply;
        msgRecvHeader.len = msgSendHeader->len;
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_, msgRecvHeader.len),
            boost::bind(&session::handle_msg_content, this, 
                boost::asio::placeholders::error));
    }
    void handle_msg_content(const boost::system::error_code& error) {
        if (error) {
            reset();
            return;
        }
        auto it = user_session.find(touser_);
        if (it != user_session.end()) {
            socket2_ = &it->second->socket_;
            boost::asio::async_write(*socket2_,
                boost::asio::buffer(&msgRecvHeader, sizeof(msgRecvHeader)),
                boost::bind(&session::handle_send_content, this, boost::asio::placeholders::error));
        }
        //Write to db
        using namespace std::chrono;
        std::string stmt = "{CALL insert_msg(?, " + 
            std::to_string(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()) + ',' +
            std::to_string(userid) + ',' + 
            std::to_string(msgRecvHeader.reply) + ',' +
            escape(buf_) + ',' +
            std::to_string(touser_) + ")}";
        msgRecvHeader.msgid = insert_auto_inc<msgid_t>(stmt.c_str());
    }
    void handle_send_content(const boost::system::error_code& error) {
        if (error) {
            reset();
            return;
        }
        boost::asio::async_write(*socket2_,
            boost::asio::buffer(buf_, sizeof(msgRecvHeader.len)),
            boost::bind(&session::listen_request, this, boost::asio::placeholders::error));
    }
    void reset() {
        user_session.erase(userid);
        delete this;
    }

private:
    ssl_socket socket_;
    
    userid_t userid;

    const static size_t bufsize = MAX_CONTENT_LEN;
    char buf_[bufsize];
    static_assert(sizeof(enum C2S) <= bufsize &&
        sizeof(struct MsgSendHeader) <= bufsize);
    //session *to;
    userid_t touser_;
    //size_t left;
    ssl_socket* socket2_;
    struct MsgRecvHeader msgRecvHeader;
};

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
