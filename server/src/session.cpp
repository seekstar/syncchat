#include "session.h"

#include <unordered_map>

#include "escape.h"
#include "odbc.h"

std::unordered_map<userid_t, session*> user_session;

session::session(boost::asio::io_service& io_service,
        boost::asio::ssl::context& context)
      : socket_(io_service, context)
{
    //to = NULL;
}

ssl_socket::lowest_layer_type&
session::socket() {
    return socket_.lowest_layer();
}

void session::start() {
    socket_.async_handshake(boost::asio::ssl::stream_base::server,
        boost::bind(&session::handshake_done, this,
        boost::asio::placeholders::error));
}
void session::handshake_done(const boost::system::error_code& error) {
    if (error) {
        delete this;
        return;
    }
    user_session[userid] = this;
    listen_request(boost::system::errc::make_error_code(boost::system::errc::success));
}
void session::listen_request(const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    boost::asio::async_read(socket_,
        boost::asio::buffer(buf_, sizeof(enum C2S)),
        boost::bind(&session::handle_request, this, boost::asio::placeholders::error));
}
void session::send_type(S2C type) {
    boost::asio::async_write(socket_,
            boost::asio::buf)
}
void session::handle_request(const boost::system::error_code& error) {
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
    case C2S::SIGNUP:
    case C2S::LOGIN:
        send_type(S2C::FAIL);
    }
}
void session::handle_msg(const boost::system::error_code& error) {
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
void session::handle_msg_content(const boost::system::error_code& error) {
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
void session::handle_send_content(const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    boost::asio::async_write(*socket2_,
        boost::asio::buffer(buf_, sizeof(msgRecvHeader.len)),
        boost::bind(&session::listen_request, this, boost::asio::placeholders::error));
}
void session::reset() {
    user_session.erase(userid);
    delete this;
}