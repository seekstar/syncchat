#include "session.h"

#include <unordered_map>
#include <random>

#include "escape.h"
#include "odbcbase.h"
#include "cppbase.h"
#include "myrandom.h"
#include "mychrono.h"

#include "srvtypes.h"

std::unordered_map<userid_t, session*> user_session;

session::session(boost::asio::io_service& io_service,
        boost::asio::ssl::context& context)
      : socket_(io_service, context),
        busy(false)
{
    //to = NULL;
}

ssl_socket::lowest_layer_type&
session::socket() {
    return socket_.lowest_layer();
}

//TODO: Support call back function to avoid copy when not busy
void session::SendLater(void *data, size_t len) {
    if (busy) {
        size_t oldsz = sendbuf.size();
        sendbuf.resize(oldsz + len);
        memcpy(sendbuf.data() + oldsz, data, len);
    } else {
        sending.resize(len);
        memcpy(sending.data(), data, len);
        dbgcout << "sending " << len << " bytes\n";
        boost::asio::async_write(socket_,
            boost::asio::buffer(sending),
            boost::bind(&session::handle_send, this, boost::asio::placeholders::error));
    }
}
void session::StartSend() {
    swap(sendbuf, sending);
    dbgcout << "sending " << sending.size() << " bytes\n";
    boost::asio::async_write(socket_,
        boost::asio::buffer(sending),
        boost::bind(&session::handle_send, this, boost::asio::placeholders::error));
}
void session::handle_send(const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    sending.clear();
    if (sendbuf.empty()) {
        busy = false;
    } else {
        StartSend();
    }
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

void session::start() {
    socket_.async_handshake(boost::asio::ssl::stream_base::server,
        boost::bind(&session::listen_signup_or_login, this,
        boost::asio::placeholders::error));
}
void session::listen_signup_or_login() {
    dbgcout << "listen_signup_or_login\n";
    boost::asio::async_read(socket_,
        boost::asio::buffer(buf_, sizeof(C2SHeader)),
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
    dbgcout << "handle_signup_or_login\n";
    struct C2SHeader *header = reinterpret_cast<struct C2SHeader *>(buf_);
    switch (header->type) {
    case C2S::IMALIVE:
        dbgcout << "client alive\n";
        break;
    case C2S::SIGNUP:
        dbgcout << "signup request\n";
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_, sizeof(SignupHeader)),
            boost::bind(&session::HandleSignupHeader, this, 
                header->tsid, boost::asio::placeholders::error));
        break;
    case C2S::LOGIN:
        dbgcout << "login request\n";
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_, sizeof(LoginInfo)),
            boost::bind(&session::handle_login, this, header->tsid,
                boost::asio::placeholders::error));
        break;
    default:
        dbgcout << "handle_signup_or_login: Unknown type " << *(C2SBaseType*)buf_ << std::endl;
        break;
    }
}
void session::HandleSignupHeader(transactionid_t tsid, const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    
    struct SignupHeader *header = reinterpret_cast<struct SignupHeader *>(buf_);
    if (header->namelen > MAX_USERNAME_LEN) {
        SendType(tsid, S2C::USERNAME_TOO_LONG);
        listen_signup_or_login();
        return;
    }
    if (header->phonelen > MAX_PHONE_LEN) {
        SendType(tsid, S2C::PHONE_TOO_LONG);
        listen_signup_or_login();
        return;
    }

    boost::asio::async_read(socket_,
        boost::asio::buffer(
            buf_ + sizeof(SignupHeader) + 2 * SHA256_DIGEST_LENGTH + sizeof(S2CHeader) + sizeof(SignupReply), 
            header->namelen + header->phonelen),
        boost::bind(&session::HandleSignup, this, tsid, boost::asio::placeholders::error));
}
void session::HandleSignup(transactionid_t tsid, const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    struct SignupHeader *header = reinterpret_cast<struct SignupHeader *>(buf_);
    uint8_t *salt = buf_ + sizeof(SignupHeader);
    uint8_t *res = salt + SHA256_DIGEST_LENGTH;
    struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(res + SHA256_DIGEST_LENGTH);
    struct SignupReply *signupreply = reinterpret_cast<struct SignupReply *>(s2cHeader + 1);
    char *name = reinterpret_cast<char*>(signupreply + 1);
    char *phone = name + header->namelen;

    genrand(salt, SHA256_DIGEST_LENGTH);
    SHA256(header->pwsha256, 2 * SHA256_DIGEST_LENGTH, res);

    using namespace std::chrono;
    s2cHeader->tsid = tsid;
    s2cHeader->type = S2C::SIGNUP_RESP;
    signupreply->day = duration_cast<days>(system_clock::now().time_since_epoch()).count();

    SQLLEN length;
    SQLRETURN retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_OUTPUT, SQL_C_UBIGINT,
        SQL_BIGINT, 0, 0, &signupreply->id, 0, &length);
    if (!SQL_SUCCEEDED(retcode)) {
        std::cerr << "HandleSignup: SQLBindParameter of user id Failed" << std::endl;
        //listen_signup_or_login();
        reset();
        return;
    }
    retcode = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_BINARY,
        SQL_BINARY, SHA256_DIGEST_LENGTH, 0, salt, SHA256_DIGEST_LENGTH, NULL);
    if (!SQL_SUCCEEDED(retcode)) {
        std::cerr << "HandleSignup: SQLBindParameter of user salt Failed" << std::endl;
        //listen_signup_or_login();
        reset();
        return;
    } 
    retcode = SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_BINARY,
        SQL_BINARY, SHA256_DIGEST_LENGTH, 0, res, SHA256_DIGEST_LENGTH, NULL);
    if (!SQL_SUCCEEDED(retcode)) {
        std::cerr << "HandleSignup: SQLBindParameter of user pw Failed" << std::endl;
        //listen_signup_or_login();
        reset();
        return;
    }
    std::string stmt = "{CALL insert_user(?, " +
        std::to_string(signupreply->day) + ',' +
        '"' + escape(name, header->namelen) + "\"," +
        '"' + escape(phone, header->phonelen) + "\"," +
        "?, ?)}";
    if (odbc_exec(std::cerr, stmt.c_str())) {
        reset();
        //listen_signup_or_login();
        return;
    }
    dbgcout << "Signup ok. id = " << signupreply->id << '\n';
    SendLater(s2cHeader, sizeof(S2CHeader) + sizeof(SignupReply));
    listen_signup_or_login();
    // boost::asio::async_write(socket_,
    //     boost::asio::buffer(s2cHeader, sizeof(S2CHeader) + sizeof(SignupReply)),
    //     boost::bind(&session::listen_signup_or_login, this, boost::asio::placeholders::error));
}
void session::handle_login(transactionid_t tsid, const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    struct LoginInfo *loginInfo = reinterpret_cast<struct LoginInfo *>(buf_);
    uint8_t *salt = buf_ + sizeof(LoginInfo);
    uint8_t *pw = salt + SHA256_DIGEST_LENGTH;
    uint8_t *res = pw + SHA256_DIGEST_LENGTH;

    std::string stmt("SELECT salt FROM user WHERE userid = " + 
        std::to_string(loginInfo->userid) + ';');
    if (odbc_exec(std::cerr, stmt.c_str())
    ) {
        std::cerr << "Error: handle_login: odbc_exec " << stmt << '\n';
        reset();
        return;
    }
    SQLLEN length;
    SQLBindCol(hstmt, 1, SQL_C_BINARY, salt, SHA256_DIGEST_LENGTH, &length);
    SQLBindCol(hstmt, 2, SQL_C_BINARY, pw, SHA256_DIGEST_LENGTH, &length);
    if (SQL_NO_DATA == SQLFetch(hstmt)) {
        dbgcout << "NO_SUCH_USER\n";
        SendType(tsid, S2C::NO_SUCH_USER);
        listen_signup_or_login();
        return;
    }
    SHA256(loginInfo->pwsha256, 2 * SHA256_DIGEST_LENGTH, res);
    if (memcmp(pw, res, SHA256_DIGEST_LENGTH)) {
        dbgcout << "WRONG_PASSWORD\n";
        SendType(tsid, S2C::WRONG_PASSWORD);
        listen_signup_or_login();
        return;
    }
    dbgcout << "LOGIN_OK\n";
    SendType(tsid, S2C::LOGIN_OK);
    userid = loginInfo->userid;
    user_session[userid] = this;
    listen_request();
}

void session::listen_request() {
    boost::asio::async_read(socket_,
        boost::asio::buffer(buf_, sizeof(C2SHeader)),
        boost::bind(&session::handle_request, this, boost::asio::placeholders::error));
}
void session::listen_request(const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    listen_request();
}
void session::handle_request(const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    dbgcout << "Received a request\n";
    struct C2SHeader *header = reinterpret_cast<struct C2SHeader *>(buf_);
    switch (header->type) {
    case C2S::MSG:
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_ + sizeof(C2SHeader) + sizeof(MsgS2CReply), sizeof(MsgC2SHeader)),
            boost::bind(&session::handle_msg, this, boost::asio::placeholders::error));
        break;
    case C2S::SIGNUP:
    case C2S::LOGIN:
        SendType(header->tsid, S2C::ALREADY_LOGINED);
        break;
    default:
        dbgcout << *(C2SBaseType*)buf_;
        listen_request();
        break;
    }
}
void session::handle_msg(const boost::system::error_code& error) {
    static_assert(sizeof(C2SHeader) + sizeof(MsgS2CHeader) + MAX_CONTENT_LEN <= BUFSIZE);
    if (error) {
        reset();
        return;
    }
    struct MsgC2SHeader *msgC2SHeader = reinterpret_cast<struct MsgC2SHeader*>
        (buf_ + sizeof(C2SHeader) + sizeof(MsgS2CReply));
    if (msgC2SHeader->len > MAX_CONTENT_LEN) {
        IgnoreMsgContent(msgC2SHeader->len);
        SendType(S2C::MSG_TOO_LONG);
        return;
    }
    boost::asio::async_read(socket_,
        boost::asio::buffer(msgC2SHeader + 1, msgC2SHeader->len),
        boost::bind(&session::handle_msg_content, this,
            boost::asio::placeholders::error));
}
void session::IgnoreMsgContent(size_t len) {
    if (len) {
        //QUESTION: std::min(len, BUFSIZE) will cause a compile error. The reasion is unclear.
        size_t sz = std::min(len, sizeof(buf_));
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_, sz),
            boost::bind(&session::HandleIgnore, this, len - sz,
                boost::asio::placeholders::error));
    } else {
        listen_request();
    }
}
void session::HandleIgnore(size_t len, const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    IgnoreMsgContent(len);
}
void session::handle_msg_content(const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    struct C2SHeader *c2sHeader = reinterpret_cast<struct C2SHeader *>(buf_);
    struct MsgS2CReply *msgS2CReply = reinterpret_cast<struct MsgS2CReply *>(c2sHeader + 1);
    struct MsgC2SHeader *msgC2SHeader = reinterpret_cast<struct MsgC2SHeader*>(msgS2CReply + 1);
    uint8_t *content = reinterpret_cast<uint8_t *>(msgC2SHeader + 1);
    //Write to db
    using namespace std::chrono;
    SQLLEN msgidLen = sizeof(msgid_t);
    SQLLEN contentLen = msgC2SHeader->len;
    SQLRETURN retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_OUTPUT, SQL_C_UBIGINT, 
        SQL_BIGINT, 0, 0, &msgS2CReply->msgid, 0, &msgidLen);
    if (!SQL_SUCCEEDED(retcode)) {
        std::cerr << "Error: handle_msg_content: SQLBindParameter of msgid failed!\n";
        listen_request();
        return;
    }
    retcode = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_BINARY,
        MAX_CONTENT_LEN, 0, content, msgC2SHeader->len, &contentLen);
    if (!SQL_SUCCEEDED(retcode)) {
        std::cerr << "Error: handle_msg_content: SQLBindParameter of content failed!\n";
        listen_request();
        return;
    }
    msgS2CReply->time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    if (odbc_exec(std::cerr, ("{CALL insert_msg(?, " +
        std::to_string(msgS2CReply->time) + ',' +
        std::to_string(userid) + ',' +  //from
        std::to_string(msgC2SHeader->to) + ",?)}"
    ).c_str())) {
        listen_request();
        return;
    }
    //Response to the sender
    struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(buf_);
    s2cHeader->type = S2C::MSG_RESP;
    SendLater(buf_, sizeof(S2CHeader) + sizeof(MsgS2CReply));
    //Forward to the receiver
    auto it = user_session.find(msgC2SHeader->to);
    if (it != user_session.end()) {
        struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(buf_);
        struct MsgS2CHeader *msgS2CHeader = static_cast<struct MsgS2CHeader *>(msgS2CReply);
        s2cHeader->tsid = 0;    //push
        s2cHeader->type = S2C::MSG;
        msgS2CHeader->from = userid;
        it->second->SendLater(s2cHeader, sizeof(S2CHeader) + sizeof(MsgS2CHeader) + msgS2CHeader->len);
    }
    listen_request();
}
void session::reset() {
    user_session.erase(userid);
    delete this;
    dbgcout << "Disconnected\n";
}
