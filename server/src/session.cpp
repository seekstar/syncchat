#include "session.h"

#include <unordered_map>
#include <random>

#include "escape.h"
#include "odbc.h"
#include "cppbase.h"
#include "myrandom.h"
#include "mychrono.h"

#include "srvtypes.h"

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
        boost::bind(&session::listen_signup_or_login, this,
        boost::asio::placeholders::error));
}
void session::listen_signup_or_login() {
    boost::asio::async_read(socket_,
        boost::asio::buffer(buf_, sizeof(C2S)),
        boost::bind(&session::handle_signup_or_login, this, boost::asio::placeholders::error));
}
void session::listen_signup_or_login(const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    listen_signup_or_login();
}
void session::handle_signup_or_login(const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    switch (*(C2S*)buf_) {
    case C2S::SIGNUP:
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_, sizeof(struct SignupInfo)),
            boost::bind(&session::handle_signup, this, boost::asio::placeholders::error));
        break;
    case C2S::LOGIN:
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_, sizeof(struct logininfo)),
            boost::bind(&session::handle_login, this, boost::asio::placeholders::error));
        break;
    default:
        dbgcout << "handle_signup_or_login: Unknown type " << *(C2SBaseType*)buf_ << std::endl;
        break;
    }
}
void session::handle_signup(const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    
    struct UserBasic *userbasic = reinterpret_cast<struct UserBasic *>(buf_);
    uint8_t *pw = reinterpret_cast<uint8_t*>(buf_) + sizeof(struct UserBasic);
    uint8_t *salt = reinterpret_cast<uint8_t*>(buf_) + sizeof(struct SignupInfo);
    genrand(salt, SHA_DIGEST_LENGTH);
    uint8_t *res = salt + SHA_DIGEST_LENGTH;
    SHA256(pw, 2 * SHA_DIGEST_LENGTH, res);

    uint8_t *bufp = salt + 2 * SHA_DIGEST_LENGTH;   //The start of free space
    try {
        using namespace std::chrono;
        struct signupreply *signupreply = reinterpret_cast<struct signupreply *>(bufp + sizeof(S2C));
        signupreply->day = duration_cast<days>(system_clock::now().time_since_epoch()).count();

        SQLLEN length;
        SQLRETURN retcode = SQLBindParameter(serverhstmt, 1, SQL_PARAM_OUTPUT, SQL_C_UBIGINT,
            SQL_BIGINT, 0, 0, &signupreply->id, 0, &length);
        if (!SQL_SUCCEEDED(retcode)) {
            throw "SQLBindParameter of user id Failed";
        }
        retcode = SQLBindParameter(serverhstmt, 2, SQL_PARAM_INPUT, SQL_C_BINARY,
            SQL_BINARY, SHA_DIGEST_LENGTH, 0, salt, SHA_DIGEST_LENGTH, NULL);
        if (!SQL_SUCCEEDED(retcode)) {
            throw "SQLBindParameter of user salt Failed";
        } 
        retcode = SQLBindParameter(serverhstmt, 3, SQL_PARAM_INPUT, SQL_C_BINARY,
            SQL_BINARY, SHA_DIGEST_LENGTH, 0, res, SHA_DIGEST_LENGTH, NULL);
        if (!SQL_SUCCEEDED(retcode)) {
            throw "SQLBindParameter of user pw Failed";
        }
        std::string stmt = "{CALL insert_user(?, " +
            std::to_string(signupreply->day) + ',' +
            userbasic->name + ',' +
            "?, ?)}";
        if (odbc_exec(std::cerr, stmt.c_str())) {
            throw "odbc_exec error";
        }
        *reinterpret_cast<S2C*>(bufp) = S2C::SIGNUP_REPLY;
        boost::asio::async_write(socket_,
            boost::asio::buffer(bufp + sizeof(SignupInfo), sizeof(S2C) + sizeof(struct signupreply)),
            boost::bind(&session::listen_signup_or_login, this, boost::asio::placeholders::error));
    } catch (const char *errmsg) {
        std::cerr << "handle_signup: " << errmsg << std::endl;
        listen_signup_or_login();
    }
}
void session::handle_login(const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    user_session[userid] = this;
    listen_request();
}

void session::listen_request() {
    boost::asio::async_read(socket_,
        boost::asio::buffer(buf_, sizeof(enum C2S)),
        boost::bind(&session::handle_request, this, boost::asio::placeholders::error));
}
void session::listen_request(const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    listen_request();
}
/*void session::send_type(S2C type) {
    boost::asio::async_write(socket_,
            boost::asio::buf)
}*/
void session::handle_request(const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    switch (*(enum C2S*)buf_) {
    case C2S::MSG:
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_, sizeof(MsgC2SHeader)),
            boost::bind(&session::handle_msg, this, boost::asio::placeholders::error));
        break;
    case C2S::SIGNUP:
    case C2S::LOGIN:
        *(S2C*)buf_ = S2C::FAIL;
        *(S2CFAIL*)(buf_ + sizeof(S2C)) = S2CFAIL::ALREADY_LOGINED;
        break;
    default:
        //DBG("Unkown request type: %hu\n", *(uint16_t*)buf_);
        dbgcout << *(C2SBaseType*)buf_;
        listen_request();
        break;
    }
}
void session::handle_msg(const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    struct MsgC2SHeader *msgC2SHeader = (struct MsgC2SHeader*)(buf_);
    if (msgC2SHeader->len > MAX_CONTENT_LEN) {
        *(enum S2C*)buf_ = S2C::FAIL;
        *(enum S2CFAIL*)(buf_ + sizeof(enum S2C)) = S2CFAIL::MSG_TOO_LONG;
        boost::asio::async_write(socket_,
            boost::asio::buffer(buf_, sizeof(S2C) + sizeof(S2CFAIL)),
            boost::bind(&session::listen_request, this, boost::asio::placeholders::error));
    }
    touser_ = msgC2SHeader->to;
    msgS2CHeader.from = userid;
    msgS2CHeader.reply = msgC2SHeader->reply;
    msgS2CHeader.len = msgC2SHeader->len;
    boost::asio::async_read(socket_,
        boost::asio::buffer(buf_, msgS2CHeader.len),
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
            boost::asio::buffer(&msgS2CHeader, sizeof(msgS2CHeader)),
            boost::bind(&session::send_msg_content, this, boost::asio::placeholders::error));
    }
    //Write to db
    try {
        using namespace std::chrono;
        SQLLEN length;
        SQLRETURN retcode = SQLBindParameter(serverhstmt, 1, SQL_PARAM_OUTPUT, SQL_C_UBIGINT, 
            SQL_BIGINT, 0, 0, &msgS2CHeader.msgid, 0, &length);
        if (!SQL_SUCCEEDED(retcode)) {
            throw "SQLBindParameter of user id Failed";
        }
        std::string stmt = "{CALL insert_msg(?, " +
        std::to_string(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()) + ',' +
        std::to_string(userid) + ',' +
        std::to_string(msgS2CHeader.reply) + ',' +
        escape(buf_) + ")}";
        //TODO:
        if (odbc_exec(std::cerr, stmt.c_str())) {
            throw "odbc_exec error";
        }
        boost::asio::async_write(socket_,
            boost::asio::buffer(&msgS2CHeader, sizeof(msgS2CHeader)),
            boost::bind(&session::send_msg_content, this, boost::asio::placeholders::error));
    } catch (const char *errmsg) {
        std::cerr << errmsg << std::endl;
    }
}
void session::send_msg_content(const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    boost::asio::async_write(*socket2_,
        boost::asio::buffer(buf_, sizeof(msgS2CHeader.len)),
        boost::bind(&session::listen_request, this, boost::asio::placeholders::error));
}
void session::reset() {
    user_session.erase(userid);
    delete this;
}
