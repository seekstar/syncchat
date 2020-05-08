#include "sslmanagerbase.h"

void SslManager::signup(std::vector<uint8_t> content) {
    qDebug() << __PRETTY_FUNCTION__;
    auto buf = C2SHeaderBuf(C2S::SIGNUP);
    PushBuf(buf, content.data(), content.size());
    SendLater(buf);
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
    SendLater(buf);
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
    case S2C::NO_SUCH_USER:
        emit noSuchUser();
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

void SslManager::UserPrivateInfoReq() {
    qDebug() << __PRETTY_FUNCTION__;
    SendLater(C2SHeaderBuf(C2S::USER_PRIVATE_INFO_REQ));
}
void SslManager::HandleUserPrivateInfoHeader(const boost::system::error_code& error) {
    HANDLE_ERROR;
    auto userPrivateInfoHeader = reinterpret_cast<UserPrivateInfoHeader *>(recvbuf_);
    boost::asio::async_read(*socket_,
        boost::asio::buffer(recvbuf_ + sizeof(UserPrivateInfoHeader),
            userPrivateInfoHeader->nameLen + userPrivateInfoHeader->phoneLen),
        boost::bind(&SslManager::HandleUserPrivateInfoContent, this, boost::asio::placeholders::error));
}
void SslManager::HandleUserPrivateInfoContent(const boost::system::error_code& error) {
    HANDLE_ERROR;
    auto userPrivateInfoHeader = reinterpret_cast<UserPrivateInfoHeader *>(recvbuf_);
    char *username = reinterpret_cast<char *>(recvbuf_ + sizeof(userPrivateInfoHeader));
    char *phone = username + userPrivateInfoHeader->phoneLen;
    emit UserPrivateInfoReply(std::string(username, userPrivateInfoHeader->nameLen),
                              std::string(phone, userPrivateInfoHeader->phoneLen));
    ListenToServer();
}

void SslManager::UserPublicInfoReq(userid_t userid) {
    auto buf = C2SHeaderBuf(C2S::USER_PUBLIC_INFO_REQ);
    PushBuf(buf, &userid, sizeof(userid));
    SendLater(buf);
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

void SslManager::FindByUsername(std::__cxx11::string username) {
    auto buf = C2SHeaderBuf(C2S::FIND_BY_USERNAME);
    uint64_t len = username.length();
    PushBuf(buf, &len, sizeof(len));
    PushBuf(buf, username.c_str(), username.length());
    SendLater(buf);
    //transactionUsername_[last_tsid] = username;
}
void SslManager::HandleFindByUsernameReplyHeader(const boost::system::error_code &error) {
    HANDLE_ERROR;
    //TODO: Handle recvbuf not long enough
    uint64_t len = *reinterpret_cast<uint64_t *>(recvbuf_ + sizeof(S2CHeader));
    boost::asio::async_read(*socket_,
        boost::asio::buffer(recvbuf_ + sizeof(S2CHeader) + sizeof(uint64_t), len * sizeof(userid_t)),
        boost::bind(&SslManager::HandleFindByUsernameReplyContent, this, boost::asio::placeholders::error));
}
void SslManager::HandleFindByUsernameReplyContent(const boost::system::error_code &error) {
    qDebug() << __PRETTY_FUNCTION__;
    HANDLE_ERROR;
    //S2CHeader *s2cHeader = reinterpret_cast<S2CHeader *>(recvbuf_);
    uint64_t len = *reinterpret_cast<uint64_t *>(recvbuf_ + sizeof(S2CHeader));
    userid_t *userid = reinterpret_cast<userid_t *>(recvbuf_ + sizeof(S2CHeader) + sizeof(uint64_t));
    std::vector<userid_t> res;
    qDebug() << len << "results";
    while (len--) {
        //qDebug() << *userid;
        //qDebug("%x ", *userid);
        res.push_back(*userid);
        ++userid;
    }
    emit FindByUsernameReply(res);
    ListenToServer();
}
