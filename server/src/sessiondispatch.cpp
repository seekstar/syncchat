#include "sessionbase.h"

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
    case C2S::FIND_BY_USERNAME:
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_ + sizeof(C2SHeader), sizeof(uint64_t)),
            boost::bind(&session::HandleFindByUsernameHeader, this, boost::asio::placeholders::error));
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
    case C2S::CREATE_GROUP:
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_ + sizeof(C2SHeader), sizeof(uint64_t)),
            boost::bind(&session::HandleCreateGroupHeader, this, boost::asio::placeholders::error));
        break;
    case C2S::JOIN_GROUP:
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_, sizeof(grpid_t)),
            boost::bind(&session::HandleJoinGroup, this, boost::asio::placeholders::error));
        break;
    case C2S::GRP_INFO:
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_ + sizeof(C2SHeader), sizeof(grpid_t)),
            boost::bind(&session::HandleGrpInfoReq, this, boost::asio::placeholders::error));
        break;
    case C2S::GRPMSG:
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_ + sizeof(C2SHeader) + sizeof(S2CGrpMsgReply) + sizeof(userid_t), 
                sizeof(C2SGrpMsgHeader)),
            boost::bind(&session::HandleGrpMsg, this, boost::asio::placeholders::error));
        break;
    default:
        dbgcout << "Warning in " << __PRETTY_FUNCTION__ << ": Unexpected request type " << 
                *(C2SBaseType*)header->type << '\n';
        listen_request();
        break;
    }
}
