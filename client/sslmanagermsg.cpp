#include "sslmanagerbase.h"

void SslManager::SendToUser(userid_t userid, CppContent content) {
    qDebug() << __PRETTY_FUNCTION__;
    auto buf = C2SHeaderBuf(C2S::MSG);
    struct MsgC2SHeader header;
    header.to = userid;
    header.len = content.size();
    PushBuf(buf, &header, sizeof(header));
    PushBuf(buf, content.data(), content.size());
    SendLater(buf);
    transactionUser_[last_tsid] = userid;
    transactionContent_[last_tsid] = content;
}
void SslManager::HandleSendToUserResp() {
    struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(recvbuf_);
    auto itUser = transactionUser_.find(s2cHeader->tsid);
    if (transactionUser_.end() == itUser) {
        qDebug() << __PRETTY_FUNCTION__ << ": transaction id" << s2cHeader->tsid << "has no corresponding private message!";
        return;
    }
    userid_t userid = itUser->second;
    transactionUser_.erase(itUser);
    auto itContent = transactionContent_.find(s2cHeader->tsid);
    if (transactionContent_.end() == itContent) {
        qDebug() << __PRETTY_FUNCTION__ << ": transaction id" << s2cHeader->tsid << "has no corresponding private message!";
        return;
    }
    CppContent content = itContent->second;
    transactionContent_.erase(itContent);
    switch (s2cHeader->type) {
    case S2C::MSG_TOO_LONG:
        emit PrivateMsgTooLong(userid, content);
        ListenToServer();
        break;
    case S2C::MSG_RESP:
        boost::asio::async_read(*socket_,
            boost::asio::buffer(recvbuf_, sizeof(MsgS2CReply)),
            boost::bind(&SslManager::HandleSendToUserResp2, this, boost::asio::placeholders::error, userid, content));
        break;
    default:
        qDebug() << "Warning in " << __PRETTY_FUNCTION__ << ": Unexpected transaction code: " << (S2CBaseType)s2cHeader->type;
        break;
    }
}
void SslManager::HandleSendToUserResp2(const boost::system::error_code &error, userid_t userid, CppContent content) {
    HANDLE_ERROR;
    auto msgS2CReply = reinterpret_cast<struct MsgS2CReply *>(recvbuf_);
    emit PrivateMsgResponse(userid, content, msgS2CReply->msgid, msgS2CReply->time);
    ListenToServer();
}

void SslManager::HandlePrivateMsgHeader(const boost::system::error_code &error) {
    HANDLE_ERROR;
    auto msgS2CHeader = reinterpret_cast<MsgS2CHeader *>(recvbuf_);
    boost::asio::async_read(*socket_,
        boost::asio::buffer(recvbuf_ + sizeof(MsgS2CHeader), msgS2CHeader->len),
        boost::bind(&SslManager::HandlePrivateMsgContent, this, boost::asio::placeholders::error));
}
void SslManager::HandlePrivateMsgContent(const boost::system::error_code &error) {
    HANDLE_ERROR;
    auto msgS2CHeader = reinterpret_cast<MsgS2CHeader *>(recvbuf_);
    uint8_t *content = reinterpret_cast<uint8_t *>(msgS2CHeader + 1);
    emit PrivateMsg(msgS2CHeader->from, CppContent(content, content + msgS2CHeader->len),
                    msgS2CHeader->msgid, msgS2CHeader->time);
    ListenToServer();
}
