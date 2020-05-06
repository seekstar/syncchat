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
#define HANDLE_ERROR    \
    if (error) {        \
        reset();        \
        return;         \
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
    dbgcout << "sha256 of password: \n";
    DbgPrintHex(header->pwsha256, SHA256_DIGEST_LENGTH);
    dbgcout << "\nsalt generated is: \n";
    DbgPrintHex(salt, SHA256_DIGEST_LENGTH);
    dbgcout << "\nsha256(pwsha256, salt) is: \n";
    DbgPrintHex(res, SHA256_DIGEST_LENGTH);
    dbgcout << '\n';

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
    length = SHA256_DIGEST_LENGTH;
    retcode = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_BINARY,
        SQL_BINARY, SHA256_DIGEST_LENGTH, 0, salt, SHA256_DIGEST_LENGTH, &length);
    if (!SQL_SUCCEEDED(retcode)) {
        std::cerr << "HandleSignup: SQLBindParameter of user salt Failed" << std::endl;
        //listen_signup_or_login();
        reset();
        return;
    } 
    length = SHA256_DIGEST_LENGTH;
    retcode = SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_BINARY,
        SQL_BINARY, SHA256_DIGEST_LENGTH, 0, res, SHA256_DIGEST_LENGTH, &length);
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

    std::string stmt("SELECT salt, pw FROM user WHERE userid = " + 
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
    if (odbc_close_cursor(std::cerr)) {
        reset();
        return;
    }
    SHA256(loginInfo->pwsha256, 2 * SHA256_DIGEST_LENGTH, res);
    dbgcout << "Given sha256 of password is: \n";
    DbgPrintHex(loginInfo->pwsha256, SHA256_DIGEST_LENGTH);
    dbgcout << "\nstored salt is: \n";
    DbgPrintHex(salt, SHA256_DIGEST_LENGTH);
    dbgcout << "\nsha256(pwsha256, salt) = \n";
    DbgPrintHex(res, SHA256_DIGEST_LENGTH);
    dbgcout << "\nThe stored result is: \n";
    DbgPrintHex(pw, SHA256_DIGEST_LENGTH);
    dbgcout << '\n';
    if (memcmp(pw, res, SHA256_DIGEST_LENGTH)) {
        dbgcout << "WRONG_PASSWORD\n";
        SendType(tsid, S2C::WRONG_PASSWORD);
        listen_signup_or_login();
        return;
    }
    dbgcout << "LOGIN_OK\n";
    userid = loginInfo->userid;
    user_session[userid] = this;
    dbgcout << "user " << userid << " online now\n";
    SendType(tsid, S2C::LOGIN_OK);
    listen_request();
}

void session::listen_request() {
    dbgcout << __PRETTY_FUNCTION__ << '\n';
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
    case C2S::SIGNUP:
    case C2S::LOGIN:
        SendType(header->tsid, S2C::ALREADY_LOGINED);
        break;
    case C2S::USER_PRIVATE_INFO_REQ:
        HandleUserPrivateInfoReq();
        break;
    case C2S::USER_PUBLIC_INFO_REQ:
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_ + sizeof(C2SHeader), sizeof(userid_t)),
            boost::bind(&session::HandleUserPublicInfoReq, this, boost::asio::placeholders::error));
        break;
    case C2S::ADD_FRIEND_REQ:
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_ + sizeof(C2SHeader), sizeof(C2SAddFriendReq)),
            boost::bind(&session::HandleAddFriendReq, this, boost::asio::placeholders::error));
        break;
    case C2S::ADD_FRIEND_REPLY:
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_ + sizeof(C2SHeader), sizeof(C2SAddFriendReply)),
            boost::bind(&session::HandleAddFriendReply, this, boost::asio::placeholders::error));
        break;
    case C2S::DELETE_FRIEND:
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_, sizeof(userid_t)),
            boost::bind(&session::HandleDeleteFriend, this, boost::asio::placeholders::error));
        break;
    case C2S::ALL_FRIENDS:
        HandleAllFriendsReq();
        break;
    case C2S::MSG:
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_ + sizeof(C2SHeader) + sizeof(MsgS2CReply), sizeof(MsgC2SHeader)),
            boost::bind(&session::handle_msg, this, boost::asio::placeholders::error));
        break;
    default:
        dbgcout << "Warning in " << __PRETTY_FUNCTION__ << ": Unexpected request type " << 
                *(C2SBaseType*)buf_ << '\n';
        listen_request();
        break;
    }
}

void session::HandleUserPrivateInfoReq() {
    dbgcout << __PRETTY_FUNCTION__ << '\n';
    auto s2cHeader = reinterpret_cast<struct S2CHeader *>(buf_);
    auto userPrivateInfoHeader = reinterpret_cast<struct UserPrivateInfoHeader *>(s2cHeader + 1);
    char *username = reinterpret_cast<char *>(userPrivateInfoHeader + 1);
    char *phone = reinterpret_cast<char *>(username + MAX_USERNAME_LEN);
    //tsid is already in place
    s2cHeader->type = S2C::USER_PRIVATE_INFO;
    SQLLEN nameLen, phoneLen;
    if (odbc_exec(std::cerr, (
        "SELECT username, phone FROM user WHERE userid = " + std::to_string(userid) + ';'
    ).c_str())) {
        std::cerr << "Error in " << __PRETTY_FUNCTION__ << ": odbc_exec fail\n";
        reset();
        return;
    }
    SQLBindCol(hstmt, 1, SQL_C_CHAR, username, MAX_USERNAME_LEN, &nameLen);
    SQLBindCol(hstmt, 2, SQL_C_CHAR, phone, MAX_PHONE_LEN, &phoneLen);
    if (SQL_NO_DATA == SQLFetch(hstmt)) {
        std::cerr << "Error in " << __PRETTY_FUNCTION__ << ": the user logined disappeared in db: " << userid << '\n';
        reset();
        return;
    }
    odbc_close_cursor(std::cerr);
    if (SQL_NULL_DATA == nameLen) {
        nameLen = 0;
    }
    if (SQL_NULL_DATA == phoneLen) {
        phoneLen = 0;
    }
    userPrivateInfoHeader->nameLen = nameLen;
    userPrivateInfoHeader->phoneLen = phoneLen;
    memcpy(username + nameLen, phone, phoneLen);
    SendLater(buf_, sizeof(S2CHeader) + sizeof(UserPrivateInfoHeader) + nameLen + phoneLen);
    listen_request();
}

void session::HandleUserPublicInfoReq(const boost::system::error_code& error) {
    dbgcout << __PRETTY_FUNCTION__ << '\n';
    HANDLE_ERROR;
    S2CHeader *s2cHeader = reinterpret_cast<S2CHeader *>(buf_);
    userid_t *id = reinterpret_cast<userid_t *>(s2cHeader + 1);
    UserPublicInfoHeader *publicInfo = reinterpret_cast<UserPublicInfoHeader *>(id);
    char *username = reinterpret_cast<char *>(publicInfo + 1);

    if (odbc_exec(std::cerr, 
        ("SELECT username FROM user WHERE userid = " + std::to_string(*id)).c_str()
    )) {
        reset();
        return;
    }
    SQLLEN length;
    SQLBindCol(hstmt, 1, SQL_C_CHAR, username, MAX_USERNAME_LEN, &length);
    //TODO: Handle error case
    if (SQL_NO_DATA == SQLFetch(hstmt)) {
        SendType(S2C::NO_SUCH_USER);
        listen_request();
        return;
    }
    if (odbc_close_cursor(std::cerr)) {
        reset();
        return;
    }
    //s2cHeader->tsid is already in place
    s2cHeader->type = S2C::USER_PUBLIC_INFO;
    assert(SQL_NULL_DATA != length);
    publicInfo->nameLen = length;
    SendLater(buf_, sizeof(S2CHeader) + sizeof(UserPublicInfoHeader) + length);
    listen_request();
}
void session::HandleAddFriendReq(const boost::system::error_code& error) {
    dbgcout << __PRETTY_FUNCTION__ << '\n';
    HANDLE_ERROR;
    C2SAddFriendReq *c2sAddFriendReq = reinterpret_cast<C2SAddFriendReq *>(buf_ + sizeof(C2SHeader));
    //Check whether they are already friends
    if (odbc_exec(std::cerr, ("SELECT user2 FROM friends WHERE user1 = " + std::to_string(userid) +
        " and user2 = " + std::to_string(c2sAddFriendReq->to) + ';').c_str())
    ) {
        reset();
        return;
    }
    if (SQL_NO_DATA != SQLFetch(hstmt)) {
        if (odbc_close_cursor(std::cerr)) {
            reset();
            return;
        }
        dbgcout << "ALREADY_FRIENDS\n";
        SendType(S2C::ALREADY_FRIENDS);
        listen_request();
        return;
    }
    //TODO: write to db
    auto it = user_session.find(c2sAddFriendReq->to);
    if (user_session.end() == it) {
        dbgcout << __PRETTY_FUNCTION__ << ": user " << c2sAddFriendReq->to << " offline\n";
        listen_request();
        return;
    }
    SendType(S2C::ADD_FRIEND_SENT); //In fact send it later
    struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(buf_);
    struct S2CAddFriendReqHeader *s2cAddFriendReq = reinterpret_cast<struct S2CAddFriendReqHeader *>(s2cHeader + 1);
    char *username = reinterpret_cast<char *>(s2cAddFriendReq + 1);
    s2cHeader->tsid = 0;    //push
    s2cHeader->type = S2C::ADD_FRIEND_REQ;
    s2cAddFriendReq->from = userid;
    if (odbc_exec(std::cerr, ("SELECT username FROM user WHERE userid = " + std::to_string(userid) + ';').c_str())) {
        reset();
        return;
    }
    SQLLEN length;
    SQLBindCol(hstmt, 1, SQL_C_BINARY, username, SHA256_DIGEST_LENGTH, &length);
    if (SQL_NO_DATA == SQLFetch(hstmt)) {
        std::cerr << "Unexpected no such a user\n";
        reset();
        return;
    }
    assert(SQL_NULL_DATA != length);
    if (odbc_close_cursor(std::cerr)) {
        reset();
        return;
    }
    //dbgcout << "username of the target user is " << std::string(username, length) << '\n';
    //dbgcout << "Length of username of the target user is " << length << '\n';
    s2cAddFriendReq->nameLen = length;
    it->second->SendLater(buf_, sizeof(S2CHeader) + sizeof(S2CAddFriendReqHeader) + length);
    listen_request();
}
void session::HandleAddFriendReply(const boost::system::error_code& error) {
    dbgcout << __PRETTY_FUNCTION__ << '\n';
    //TODO: read from db to make sure that there is such an add friend request
    HANDLE_ERROR;
    S2CHeader *s2cHeader = reinterpret_cast<S2CHeader *>(buf_);
    C2SAddFriendReply *c2sAddFriendReply = reinterpret_cast<C2SAddFriendReply *>(s2cHeader + 1);
    s2cHeader->tsid = 0;    //push
    s2cHeader->type = S2C::ADD_FRIEND_REPLY;
    userid_t to = c2sAddFriendReply->to;
    S2CAddFriendReply *s2cAddFriendReply = reinterpret_cast<S2CAddFriendReply *>(c2sAddFriendReply);
    s2cAddFriendReply->from = userid;
    auto it = user_session.find(to);
    if (user_session.end() == it) {
        dbgcout << __PRETTY_FUNCTION__ << ": user " << c2sAddFriendReply->to << " offline\n";
        listen_request();
        return;
    }
    it->second->SendLater(buf_, sizeof(S2CHeader) + sizeof(S2CAddFriendReply));
    if (s2cAddFriendReply->reply) {
        //They becomes friends.
        if (odbc_exec(std::cerr, ("INSERT INTO friends VALUES(" +
            std::to_string(userid) + ',' + std::to_string(to) + "),(" +
            std::to_string(to) + ',' + std::to_string(userid) + ");").c_str())
        ) {
            reset();
            return;
        }
    }
    listen_request();
}

void session::HandleDeleteFriend(const boost::system::error_code& error) {
    HANDLE_ERROR;
    //userid_t friendId = *reinterpret_cast<userid_t *>(s2cHeader + 1);
    userid_t friendId = *reinterpret_cast<userid_t *>(buf_);
    if (odbc_exec(std::cerr, ("DELETE FROM friends WHERE user1 = " + std::to_string(userid) +
        " and user2 = " + std::to_string(friendId) + ';'
    ).c_str())) {
        std::cerr << "Error in " << __PRETTY_FUNCTION__ << ": odbc_exec failed\n";
        reset();
        return;
    }
    if (odbc_exec(std::cerr, ("DELETE FROM friends WHERE user1 = " + std::to_string(friendId) +
        " and user2 = " + std::to_string(userid) + ';'
    ).c_str())) {
        std::cerr << "Error in " << __PRETTY_FUNCTION__ << ": odbc_exec failed\n";
        reset();
        return;
    }

    S2CHeader *s2cHeader = reinterpret_cast<S2CHeader *>(buf_);
    userid_t *peerId = reinterpret_cast<userid_t *>(s2cHeader + 1);
    s2cHeader->tsid = 0;    //push
    s2cHeader->type = S2C::DELETE_FRIEND;
    *peerId = userid;
    //TODO: Handle the offline case
    auto it = user_session.find(friendId);
    if (it != user_session.end()) {
        it->second->SendLater(buf_, sizeof(S2CHeader) + sizeof(userid_t));
    }
    listen_request();
}

void session::HandleAllFriendsReq() {
    dbgcout << __PRETTY_FUNCTION__ << '\n';
    struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(buf_);
    uint64_t *num = reinterpret_cast<uint64_t *>(s2cHeader + 1);
    userid_t *friendIdStart = reinterpret_cast<userid_t *>(num + 1);
    s2cHeader->tsid = 0;    //push
    s2cHeader->type = S2C::FRIENDS;
    if (odbc_exec(std::cerr, ("SELECT user2 FROM friends WHERE user1 = " + std::to_string(userid) + ';').c_str())) {
        std::cerr << "Error in " << __PRETTY_FUNCTION__ << ": odbc_exec fail\n";
        reset();
        return;
    }
    *num = 0;
    userid_t *friendId = friendIdStart;
    while (1) {
        if (reinterpret_cast<uint8_t*>(friendId + 1) > buf_ + BUFSIZE) {
            SendLater(buf_, reinterpret_cast<uint8_t*>(friendId) - buf_);
            friendId = friendIdStart;
            *num = 0;
        }
        SQLLEN length;
        SQLBindCol(hstmt, 1, SQL_C_UBIGINT, friendId, sizeof(userid_t), &length);
        if (SQL_NO_DATA != SQLFetch(hstmt)) {
            ++*num;
            ++friendId;
        } else {
            if (*num) {
                SendLater(buf_, reinterpret_cast<uint8_t*>(friendId) - buf_);
            }
            break;
        }
    }
    listen_request();
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
void session::handle_sslio_error(const boost::system::error_code& error) {
    SslIO::handle_sslio_error(error);
    reset();
}
