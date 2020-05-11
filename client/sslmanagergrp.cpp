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
    emit NewGrpWithName(createGroupReply->grpid, it->second);
    transactionGroupName_.erase(it);
    ListenToServer();
}

void SslManager::JoinGroup(grpid_t grpid) {
    qDebug() << __PRETTY_FUNCTION__;
    auto buf = C2SHeaderBuf_noresp(C2S::JOIN_GROUP);
    PushBuf(buf, &grpid, sizeof(grpid));
    SendLater(buf);
}
void SslManager::HandleJoinGroupReply(const boost::system::error_code& error) {
    //qDebug() << __PRETTY_FUNCTION__;
    HANDLE_ERROR;
    auto grpid = *reinterpret_cast<grpid_t *>(recvbuf_);
    emit NewGroup(grpid);
    ListenToServer();
}

void SslManager::GrpInfoReq(grpid_t grpid) {
    qDebug() << "Requiring information of group" << grpid;
    auto buf = C2SHeaderBuf(C2S::GRP_INFO);
    PushBuf(buf, &grpid, sizeof(grpid));
    SendLater(buf);
    transactionGrp_[last_tsid] = grpid;
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
    emit GrpInfoReply(it->second, std::string(grpname, len));
    transactionGrp_.erase(it);
    ListenToServer();
}

void SslManager::AllGrps() {
    qDebug() << __PRETTY_FUNCTION__;
    SendLater(C2SHeaderBuf_noresp(C2S::ALL_GROUPS));
}
void SslManager::HandleGrpsHeader(const boost::system::error_code& error) {
    HANDLE_ERROR;
    uint64_t num = *reinterpret_cast<uint64_t *>(recvbuf_);
    HandleGrpsContentNoError(num);
}
void SslManager::HandleGrpsContent(uint64_t num, const boost::system::error_code &error) {
    HANDLE_ERROR;
    HandleGrpsContentNoError(num);
}
void SslManager::HandleGrpsContentNoError(uint64_t num) {
    constexpr uint64_t MAXNUM = RECVBUFSIZE / sizeof(grpid_t);
    uint64_t readNum = std::min(num, MAXNUM);
    boost::asio::async_read(*socket_,
        boost::asio::buffer(recvbuf_, readNum * sizeof(grpid_t)),
        boost::bind(&SslManager::HandleGrpsContentMore, this,
                    num - readNum, readNum, boost::asio::placeholders::error));
}
void SslManager::HandleGrpsContentMore(uint64_t num, uint64_t readNum, const boost::system::error_code &error) {
    HANDLE_ERROR;
    grpid_t *grps = reinterpret_cast<grpid_t *>(recvbuf_);
    emit Grps(std::vector<grpid_t>(grps, grps + readNum));
    if (num) {
        HandleGrpsContentNoError(num);
    } else {
        ListenToServer();
    }
}

void SslManager::ChangeGrpOwner(grpid_t grpid, userid_t userid) {
    auto buf = C2SHeaderBuf_noresp(C2S::CHANGE_GROUP_OWNER);
    PushBuf(buf, &grpid, sizeof(grpid));
    PushBuf(buf, &userid, sizeof(userid));
    SendLater(buf);
}

void SslManager::AllGrpMember(grpid_t grpid) {
    auto buf = C2SHeaderBuf_noresp(C2S::ALL_GROUP_MEMBER);
    PushBuf(buf, &grpid, sizeof(grpid));
    SendLater(buf);
}
