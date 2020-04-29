#ifndef SESSION_H_
#define SESSION_H_

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "types.h"

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

class session {
public:
    session(boost::asio::io_service& io_service,
        boost::asio::ssl::context& context);

    ssl_socket::lowest_layer_type& socket();

    void start();

private:
    void listen_signup_or_login(const boost::system::error_code& error);
    void handle_signup_or_login(const boost::system::error_code& error);
    void handle_signup(const boost::system::error_code& error);
    void handle_login(const boost::system::error_code& error);
    
    void listen_request();
    void listen_request(const boost::system::error_code& error);
    void handle_request(const boost::system::error_code& error);
    void handle_msg(const boost::system::error_code& error);
    void handle_msg_content(const boost::system::error_code& error);
    void handle_send_content(const boost::system::error_code& error);
    void reset();
    
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

#endif //SESSION_H_