#include "sslmanagerbase.h"

void SslManager::SendToGroup(grpid_t grpid, msgcontent_t content) {
    qDebug() << __PRETTY_FUNCTION__;
    auto buf = C2SHeaderBuf(C2S::GRPMSG);
    struct C2SGrpMsgHeader c2sGroupMsgHeader;
    c2sGroupMsgHeader.to = grpid;
    c2sGroupMsgHeader.len = content.size();
    PushBuf(buf, &c2sGroupMsgHeader, sizeof(c2sGroupMsgHeader));
    PushBuf(buf, content.data(), content.size());
    SendLater(buf);
    transactionGrp_[last_tsid] = grpid;
    transactionContent_[last_tsid] = content;
}
void SslManager::HandleGrpMsgReply(const boost::system::error_code& error) {
    HANDLE_ERROR;
    auto s2cHeader = reinterpret_cast<struct S2CHeader *>(recvbuf_);
    auto s2cGrpMsgReply = reinterpret_cast<S2CGrpMsgReply *>(s2cHeader + 1);
    auto grpidIt = transactionGrp_.find(s2cHeader->tsid);
    if (transactionGrp_.end() == grpidIt) {
        qDebug() << "Error in" << __PRETTY_FUNCTION__ << ": The transaction id" << s2cHeader->tsid << "has no corresponding group id";
        ListenToServer();
    }
    auto contentIt = transactionContent_.find(s2cHeader->tsid);
    if (transactionContent_.end() == contentIt) {
        qDebug() << "Error in" << __PRETTY_FUNCTION__ << ": The transaction id" << s2cHeader->tsid << "has no corresponding content";
        ListenToServer();
    }
    emit GrpMsgResp(s2cGrpMsgReply->grpmsgid, s2cGrpMsgReply->time, grpidIt->second, contentIt->second);
    ListenToServer();
}
void SslManager::HandleGrpMsgHeader(const boost::system::error_code &error) {
    HANDLE_ERROR;
    auto s2cMsgGrpHeader = reinterpret_cast<struct S2CMsgGrpHeader *>(recvbuf_);
    boost::asio::async_read(*socket_,
        boost::asio::buffer(recvbuf_ + sizeof(S2CMsgGrpHeader), s2cMsgGrpHeader->len),
        boost::bind(&SslManager::HandleGrpMsgContent, this, boost::asio::placeholders::error));
}
void SslManager::HandleGrpMsgContent(const boost::system::error_code &error) {
    HANDLE_ERROR;
    auto s2cMsgGrpHeader = reinterpret_cast<struct S2CMsgGrpHeader *>(recvbuf_);
    uint8_t *content = reinterpret_cast<uint8_t *>(s2cMsgGrpHeader + 1);
    emit GrpMsg(s2cMsgGrpHeader->grpmsgid, s2cMsgGrpHeader->time, s2cMsgGrpHeader->from,
                s2cMsgGrpHeader->to, msgcontent_t(content, content + s2cMsgGrpHeader->len));
    ListenToServer();
}
