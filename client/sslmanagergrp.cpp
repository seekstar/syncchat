#include "sslmanagerbase.h"

void SslManager::CreateGroup(std::__cxx11::string grpname) {
    auto buf = C2SHeaderBuf(C2S::CREATE_GROUP);
    uint64_t len = grpname.length();
    PushBuf(buf, &len, sizeof(len));
    PushBuf(buf, grpname.c_str(), grpname.length());
    SendLater(buf);
    transactionGroupName_[last_tsid] = grpname;
}
void SslManager::HandleCreateGroupReply(const boost::system::error_code &error) {
    HANDLE_ERROR;
    auto s2cHeader = reinterpret_cast<struct S2CHeader *>(recvbuf_);
    auto createGroupReply = reinterpret_cast<struct CreateGroupReply *>(s2cHeader + 1);
    auto it = transactionGroupName_.find(s2cHeader->tsid);
    if (transactionGroupName_.end() == it) {
        qDebug() << "Warning in" << __PRETTY_FUNCTION__ << ": transaction id" << s2cHeader->tsid <<
                    "has no corresponding group name";
        ListenToServer();
        return;
    }
    emit NewGroup(createGroupReply->grpid, it->second);
    transactionGroupName_.erase(it);
    ListenToServer();
}

void SslManager::JoinGroup(grpid_t grpid) {
    qDebug() << __PRETTY_FUNCTION__;
    auto buf = C2SHeaderBuf_noreply(C2S::JOIN_GROUP);
    PushBuf(buf, &grpid, sizeof(grpid));
    SendLater(buf);
}
void SslManager::HandleJoinGroupReply(const boost::system::error_code& error) {
    //qDebug() << __PRETTY_FUNCTION__;
    HANDLE_ERROR;
    auto grpid = *reinterpret_cast<grpid_t *>(recvbuf_);
    qDebug() << "Join group" << grpid << "agreed, asking for group name";
    //TODO: Let upper level to do this
    GrpInfoReq(grpid);
    transactionGrp_[last_tsid] = grpid;
    ListenToServer();
}
void SslManager::GrpInfoReq(grpid_t grpid) {
    qDebug() << __PRETTY_FUNCTION__;
    auto buf = C2SHeaderBuf(C2S::GRP_INFO);
    PushBuf(buf, &grpid, sizeof(grpid));
    SendLater(buf);
}
void SslManager::HandleGrpInfoHeader(const boost::system::error_code &error) {
    qDebug() << __PRETTY_FUNCTION__;
    HANDLE_ERROR;
    uint64_t len = *reinterpret_cast<uint64_t *>(recvbuf_ + sizeof(S2CHeader));
    boost::asio::async_read(*socket_,
        boost::asio::buffer(recvbuf_ + sizeof(S2CHeader) + sizeof(uint64_t), len),
        boost::bind(&SslManager::HandleGrpInfoContent, this, boost::asio::placeholders::error));
}
void SslManager::HandleGrpInfoContent(const boost::system::error_code &error) {
    qDebug() << __PRETTY_FUNCTION__;
    HANDLE_ERROR;
    auto s2cHeader = reinterpret_cast<S2CHeader *>(recvbuf_);
    uint64_t len = *reinterpret_cast<uint64_t *>(recvbuf_ + sizeof(S2CHeader));
    char *grpname = reinterpret_cast<char *>(recvbuf_ + sizeof(S2CHeader) + sizeof(uint64_t));
    auto it = transactionGrp_.find(s2cHeader->tsid);
    if (transactionGrp_.end() == it) {
        qDebug() << "Warning in" << __PRETTY_FUNCTION__ << ": transaction id" << s2cHeader->tsid << "has no corresponding group id";
        ListenToServer();
        return;
    }
    emit NewGroup(it->second, std::string(grpname, len));
    transactionGrp_.erase(it);
    ListenToServer();
}
