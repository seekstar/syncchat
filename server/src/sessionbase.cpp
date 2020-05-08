#include "session.h"
#include "sessionbase.h"

std::unordered_map<userid_t, session*> user_session;

session::session(io_service_t& io_service, context_t& context)
      : SslIO(io_service, context)
{
    //to = NULL;
}

ssl_socket::lowest_layer_type&
session::socket() {
    //return SslIO::socket();
    return socket_.lowest_layer();
}

void session::SendType(transactionid_t tsid, S2C type) {
    reinterpret_cast<struct S2CHeader *>(buf_)->tsid = tsid;
    SendType(type);
}
//Use it when tsid is already in place
void session::SendType(S2C type) {
    reinterpret_cast<struct S2CHeader *>(buf_)->type = type;
    SendLater(buf_, sizeof(S2CHeader));
}

void session::SendInfo(std::string info) {
    S2CHeader *s2cHeader = reinterpret_cast<S2CHeader *>(buf_);
    uint64_t *len = reinterpret_cast<uint64_t *>(s2cHeader + 1);
    s2cHeader->tsid = 0;    //push
    s2cHeader->type = S2C::INFO;
    *len = info.size();
    memcpy(len + 1, info.c_str(), *len);
    SendLater(buf_, sizeof(S2CHeader) + sizeof(uint64_t) + *len);
}

void session::start() {
    socket_.async_handshake(boost::asio::ssl::stream_base::server,
        boost::bind(&session::listen_signup_or_login, this,
        boost::asio::placeholders::error));
}

void session::reset() {
    user_session.erase(userid);
    delete this;
    dbgcout << "Disconnected\n";
}
void session::handle_sslio_error(const boost::system::error_code& error) {
    SslIO::handle_sslio_error(error);
    reset();
}
