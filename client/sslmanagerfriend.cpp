#include "sslmanagerbase.h"

void SslManager::AllFriendsReq() {
    qDebug() << __PRETTY_FUNCTION__;
    SendLater(C2SHeaderBuf_noreply(C2S::ALL_FRIENDS));  //reply as push
}
void SslManager::HandleFriendsHeader(const boost::system::error_code& error) {
    HANDLE_ERROR;
    uint64_t num = *reinterpret_cast<uint64_t *>(recvbuf_);
    HandleFriendsContentNoError(num);
}
void SslManager::HandleFriendsContent(uint64_t num, const boost::system::error_code &error) {
    HANDLE_ERROR;
    HandleFriendsContentNoError(num);
}
void SslManager::HandleFriendsContentNoError(uint64_t num) {
    constexpr uint64_t MAXNUM = RECVBUFSIZE / sizeof(userid_t);
    uint64_t readNum = std::min(num, MAXNUM);
    boost::asio::async_read(*socket_,
        boost::asio::buffer(recvbuf_, readNum * sizeof(userid_t)),
        boost::bind(&SslManager::HandleFriendsContentMore, this,
                    num - readNum, readNum, boost::asio::placeholders::error));
}
void SslManager::HandleFriendsContentMore(uint64_t num, uint64_t readNum, const boost::system::error_code &error) {
    HANDLE_ERROR;
    userid_t *friends = reinterpret_cast<userid_t *>(recvbuf_);
    emit Friends(std::vector<userid_t>(friends, friends + readNum));
    if (num) {
        HandleFriendsContentNoError(num);
    } else {
        ListenToServer();
    }
}

void SslManager::AddFriend(userid_t userid) {
    auto buf = C2SHeaderBuf(C2S::ADD_FRIEND_REQ);
    PushBuf(buf, &userid, sizeof(userid));
    SendLater(buf);
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
    SendLater(buf);
}

void SslManager::HandleAddFriendReply(const boost::system::error_code& error) {
    HANDLE_ERROR;
    struct S2CAddFriendReply *s2cAddFriendReply = reinterpret_cast<struct S2CAddFriendReply *>(recvbuf_);
    emit addFriendReply(s2cAddFriendReply->from, s2cAddFriendReply->reply);
    ListenToServer();
}

void SslManager::DeleteFriend(userid_t userid) {
    auto buf = C2SHeaderBuf_noreply(C2S::DELETE_FRIEND);    //Just delete, don't reply
    PushBuf(buf, &userid, sizeof(userid));
    SendLater(buf);
}
void SslManager::HandleBeDeletedFriend(const boost::system::error_code& error) {
    HANDLE_ERROR;
    emit BeDeletedFriend(*reinterpret_cast<userid_t *>(recvbuf_));
    ListenToServer();
}
