#include "sslmanager.h"

#include <QDebug>

#include <iostream>
#include <boost/bind.hpp>

#include "pushbuf.h"
#include "cppbase.h"

SslManager::SslManager(const char *ip, const char *port)
    : //QThread(parent),
      //work(new boost::asio::io_service::work(io_service)),
      resolver(io_service),
      query(ip, port),
      iterator(resolver.resolve(query)),
      context(NULL),
      //socket_(io_service, context),
      socket_(NULL),
      busy(false),
      last_tsid(0)
{
    moveToThread(this);
    //setParent(parent);
    connect(&timer, &QTimer::timeout, [&] {
        //qDebug() << "poll";
        io_service.poll();
    });
    timer.start(100);
}

SslManager::~SslManager() {
}
//void SslManager::stop_io_service() {
//    work.reset();
//}

void SslManager::run() {
    qDebug() << "The thread id of sslManager is " << QThread::currentThreadId();
    sslconn();
//    io_service.run(); //won't return because it's listening
//    io_service.reset();
//    qDebug() << "io_service.run() returned";
    exec();
}

//void SslManager::run_io_service() {
//    qDebug() << __PRETTY_FUNCTION__;

//    if (io_service.stopped()) {
//        qDebug() << "io_service.run()";
//        io_service.run();
//        io_service.reset();
//        qDebug() << "io_service.run() returned";
//    } else {
//        qDebug() << "io_service is already running";
//    }
//}

#define HANDLE_ERROR        \
    if (error) {            \
        emit sigErr(std::string("Error: ") + __PRETTY_FUNCTION__ + "\n" + error.message());\
        busy = false;       \
        delete context;     \
        context = NULL;     \
        delete socket_;     \
        socket_ = NULL;     \
        sending.clear();    \
        sendbuf.clear();    \
        return;             \
    }

//TODO: Support call back function to avoid copy when not busy
void SslManager::SendLater(void *data, size_t len) {
    if (busy) {
        size_t oldsz = sendbuf.size();
        sendbuf.resize(oldsz + len);
        memcpy(sendbuf.data() + oldsz, data, len);
    } else {
        sending.resize(len);
        memcpy(sending.data(), data, len);
        dbgcout << "sending " << len << " bytes\n";
        boost::asio::async_write(*socket_,
            boost::asio::buffer(sending),
            boost::bind(&SslManager::handle_send, this, boost::asio::placeholders::error));
    }
}
void SslManager::StartSend() {
    swap(sendbuf, sending);
    dbgcout << "sending " << sending.size() << " bytes\n";
    boost::asio::async_write(*socket_,
        boost::asio::buffer(sending),
        boost::bind(&SslManager::handle_send, this, boost::asio::placeholders::error));
}
void SslManager::handle_send(const boost::system::error_code& error) {
    HANDLE_ERROR;
    sending.clear();
    if (sendbuf.empty()) {
        busy = false;
    } else {
        StartSend();
    }
}
std::vector<uint8_t> SslManager::C2SHeaderBuf(C2S type) {
    std::vector<uint8_t> buf(sizeof(C2SHeader));
    struct C2SHeader *c2sHeader = reinterpret_cast<struct C2SHeader *>(buf.data());
    c2sHeader->tsid = ++last_tsid;
    c2sHeader->type = type;
    transactionType_[last_tsid] = type;
    return buf;
}

void SslManager::handle_handshake(const boost::system::error_code& error)
{
    HANDLE_ERROR;
    qDebug() << "handshake done";
    emit sigDone();
    busy = false;
    ListenToServer();
}

void SslManager::handle_connect(const boost::system::error_code& error)
{
    //std::cerr << "After async call, the thread id is " << std::this_thread::get_id() << std::endl;
    qDebug() << "Enter handle_connect";
    HANDLE_ERROR;
    socket_->async_handshake(boost::asio::ssl::stream_base::client,
                             boost::bind(&SslManager::handle_handshake, this,
                                         boost::asio::placeholders::error));
}

bool SslManager::verify_certificate(bool preverified,
                        boost::asio::ssl::verify_context& ctx)
{
    // The verify callback can be used to check whether the certificate that is
    // being presented is valid for the peer. For example, RFC 2818 describes
    // the steps involved in doing this for HTTPS. Consult the OpenSSL
    // documentation for more details. Note that the callback is called once
    // for each certificate in the certificate chain, starting from the root
    // certificate authority.

    // In this example we will simply print the certificate's subject name.
    char subject_name[256];
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    qDebug() << "Verifying" << subject_name;

    return preverified;
}

void SslManager::sslconn() {
    qDebug() << "sslconn";
    if (busy) {
        qDebug() << "SslManager busy";
        return;
    }
    busy = true;

    //std::cerr << "The thread id now is " << std::this_thread::get_id() << std::endl;

    if (context) {
        delete context;
    }
    context = new context_t(boost::asio::ssl::context::sslv23);
    context->load_verify_file("ca.pem");
    if (socket_) {
        delete socket_;
    }
    socket_ = new ssl_socket(io_service, *context);
    socket_->set_verify_mode(boost::asio::ssl::verify_peer);
    socket_->set_verify_callback(boost::bind(&SslManager::verify_certificate, this, _1, _2));
    qDebug() << "before async_connect";
    boost::asio::async_connect(socket_->lowest_layer(), iterator,
                               boost::bind(&SslManager::handle_connect, this, boost::asio::placeholders::error));
    //run_io_service();
}

void SslManager::ListenToServer() {
    boost::asio::async_read(*socket_,
        boost::asio::buffer(recvbuf_, sizeof(S2CHeader)),
        boost::bind(&SslManager::HandleS2CHeader, this, boost::asio::placeholders::error));
    //run_io_service();
}
void SslManager::HandleS2CHeader(const boost::system::error_code& error) {
    HANDLE_ERROR;
    struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(recvbuf_);
    if (0 == s2cHeader->tsid) {
        switch (s2cHeader->type) {
        case S2C::ADD_FRIEND_REQ:
            boost::asio::async_read(*socket_,
                boost::asio::buffer(recvbuf_, sizeof(S2CAddFriendReqHeader)),
                boost::bind(&SslManager::HandleAddFriendReqHeader, this, boost::asio::placeholders::error));
            break;
        case S2C::ADD_FRIEND_REPLY:
            boost::asio::async_read(*socket_,
                boost::asio::buffer(recvbuf_, sizeof(S2CAddFriendReply)),
                boost::bind(&SslManager::HandleAddFriendReply, this, boost::asio::placeholders::error));
            break;
        default:
            qWarning() << "Unexpected type in " << __PRETTY_FUNCTION__ << ": " << (S2CBaseType)s2cHeader->type;
            break;
        }
    } else {
        auto it = transactionType_.find(s2cHeader->tsid);
        if (transactionType_.end() == it) {
            qDebug() << "Warning in " << __PRETTY_FUNCTION__ << ": received an unknown transaction id from server: " << s2cHeader->tsid;
            ListenToServer();
            return;
        }
        switch (it->second) {
        case C2S::SIGNUP:
            HandleSignupReply();
            break;
        case C2S::LOGIN:
            HandleLoginReply();
            break;
        case C2S::USER_PUBLIC_INFO_REQ:
            boost::asio::async_read(*socket_,
                boost::asio::buffer(recvbuf_ + sizeof(S2CHeader), sizeof(UserPublicInfoHeader)),
                boost::bind(&SslManager::HandleUserPublicInfoHeader, this, boost::asio::placeholders::error));
            break;
        case C2S::ADD_FRIEND_REQ:
            HandleAddFriendResponse();
            break;
        default:
            qWarning() << "Unexpected transaction id: " << s2cHeader->tsid;
            break;
        }
        transactionType_.erase(it);
    }
}

void SslManager::signup(std::vector<uint8_t> content) {
    qDebug() << __PRETTY_FUNCTION__;
    auto buf = C2SHeaderBuf(C2S::SIGNUP);
    PushBuf(buf, content.data(), content.size());
    SendLater(buf.data(), buf.size());
}
void SslManager::HandleSignupReply() {
    struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(recvbuf_);
    switch(s2cHeader->type) {
    case S2C::ALREADY_LOGINED:
        emit alreadyLogined();
        break;
    case S2C::USERNAME_TOO_LONG:
        emit usernameTooLong();
        break;
    case S2C::PHONE_TOO_LONG:
        emit phoneTooLong();
        break;
    case S2C::SIGNUP_RESP:
        boost::asio::async_read(*socket_,
            boost::asio::buffer(recvbuf_, sizeof(SignupReply)),
            boost::bind(&SslManager::HandleSignupReply2, this, boost::asio::placeholders::error));
        break;
    default:
        qWarning() << "Unexpected type in " << __PRETTY_FUNCTION__ << ": " << (S2CBaseType)s2cHeader->type;
        break;
    }
}
void SslManager::HandleSignupReply2(const boost::system::error_code& error) {
    HANDLE_ERROR;
    struct SignupReply *signupReply = reinterpret_cast<struct SignupReply *>(recvbuf_);
    emit signupDone(signupReply->id);
    ListenToServer();
}

void SslManager::login(struct LoginInfo loginInfo) {
    qDebug() << __PRETTY_FUNCTION__;
    auto buf = C2SHeaderBuf(C2S::LOGIN);
    PushBuf(buf, &loginInfo, sizeof(loginInfo));
    SendLater(buf.data(), buf.size());
    transactionUser_[*reinterpret_cast<transactionid_t*>(buf.data())] = loginInfo.userid;
}
void SslManager::HandleLoginReply() {
    struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(recvbuf_);
    TransactionUserMap::iterator it;
    switch (s2cHeader->type) {
    case S2C::ALREADY_LOGINED:
        //qDebug() << "Warning: ALREADY_LOGINED";
        emit alreadyLogined();
        break;
    case S2C::WRONG_PASSWORD:
        emit wrongPassword();
        break;
    case S2C::LOGIN_OK:
        it = transactionUser_.find(s2cHeader->tsid);
        if (transactionUser_.end() == it) {
            qDebug() << "Error in " << __PRETTY_FUNCTION__ << ": Transaction id doesn't have corresponding user id";
            break;
        }
        emit loginDone(it->second);
        transactionUser_.erase(it);
        break;
    default:
        qWarning() << "Unexpected type in " << __PRETTY_FUNCTION__ << ": " << (S2CBaseType)s2cHeader->type;
        break;
    }
    ListenToServer();
}

void SslManager::UserPublicInfoReq(userid_t userid) {
    auto buf = C2SHeaderBuf(C2S::USER_PUBLIC_INFO_REQ);
    PushBuf(buf, &userid, sizeof(userid));
    SendLater(buf.data(), buf.size());
    transactionUser_[last_tsid] = userid;
}
void SslManager::HandleUserPublicInfoHeader(const boost::system::error_code& error) {
    HANDLE_ERROR;
    auto s2cHeader = reinterpret_cast<S2CHeader*>(recvbuf_);
    auto userPublicInfoHeader = reinterpret_cast<UserPublicInfoHeader *>(s2cHeader + 1);
    boost::asio::async_read(*socket_,
        boost::asio::buffer(recvbuf_ + sizeof(S2CHeader) + sizeof(UserPublicInfoHeader), userPublicInfoHeader->nameLen),
        boost::bind(&SslManager::HandleUserPublicInfoContent, this, boost::asio::placeholders::error));
}
void SslManager::HandleUserPublicInfoContent(const boost::system::error_code& error) {
    HANDLE_ERROR;
    auto s2cHeader = reinterpret_cast<S2CHeader*>(recvbuf_);
    auto userPublicInfoHeader = reinterpret_cast<UserPublicInfoHeader *>(s2cHeader + 1);
    char *username = reinterpret_cast<char *>(userPublicInfoHeader + 1);
    auto it = transactionUser_.find(s2cHeader->tsid);
    if (transactionUser_.end() == it) {
        qWarning() << "Warning in" << __PRETTY_FUNCTION__ << ": Received a transaction id which do not correspond to a query";
    } else {
        emit UserPublicInfoReply(it->second, std::string(username, userPublicInfoHeader->nameLen));
        transactionUser_.erase(it);
    }
    ListenToServer();
}

void SslManager::AddFriend(userid_t userid) {
    auto buf = C2SHeaderBuf(C2S::ADD_FRIEND_REQ);
    PushBuf(buf, &userid, sizeof(userid));
    SendLater(buf.data(), buf.size());
}
void SslManager::HandleAddFriendResponse() {
    struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(recvbuf_);
    switch (s2cHeader->type) {
    case S2C::ALREADY_FRIENDS:
        emit alreadyFriends();
        break;
    case S2C::ADD_FRIEND_SENT:
        emit addFriendSent();
        break;
    default:
        qWarning() << "Unexpected type in " << __PRETTY_FUNCTION__ << ": " << (S2CBaseType)s2cHeader->type;
        break;
    }
    ListenToServer();
}

void SslManager::HandleAddFriendReqHeader(const boost::system::error_code& error) {
    qDebug() << __PRETTY_FUNCTION__;
    HANDLE_ERROR;
    auto s2cAddFriendReqHeader = reinterpret_cast<S2CAddFriendReqHeader *>(recvbuf_);
    qDebug() << "The length of username is " << s2cAddFriendReqHeader->nameLen;
    boost::asio::async_read(*socket_,
        boost::asio::buffer(s2cAddFriendReqHeader + 1, s2cAddFriendReqHeader->nameLen),
        boost::bind(&SslManager::HandleAddFriendReqContent, this, boost::asio::placeholders::error));
}
void SslManager::HandleAddFriendReqContent(const boost::system::error_code& error) {
    qDebug() << __PRETTY_FUNCTION__;
    HANDLE_ERROR;
    auto s2cAddFriendReqHeader = reinterpret_cast<S2CAddFriendReqHeader *>(recvbuf_);
    char *username = reinterpret_cast<char *>(s2cAddFriendReqHeader + 1);
    emit addFriendReq(s2cAddFriendReqHeader->from, std::string(username, s2cAddFriendReqHeader->nameLen));
    ListenToServer();
}

void SslManager::ReplyAddFriend(userid_t userid, bool reply) {
    auto buf = C2SHeaderBuf(C2S::ADD_FRIEND_REPLY);
    struct C2SAddFriendReply c2sAddFriendReply;
    c2sAddFriendReply.to = userid;
    c2sAddFriendReply.reply = reply;
    PushBuf(buf, &c2sAddFriendReply, sizeof(c2sAddFriendReply));
    SendLater(buf.data(), buf.size());
}

void SslManager::HandleAddFriendReply(const boost::system::error_code& error) {
    HANDLE_ERROR;
    struct S2CAddFriendReply *s2cAddFriendReply = reinterpret_cast<struct S2CAddFriendReply *>(recvbuf_);
    emit addFriendReply(s2cAddFriendReply->from, s2cAddFriendReply->reply);
    ListenToServer();
}
