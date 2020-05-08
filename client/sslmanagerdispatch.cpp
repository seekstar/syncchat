#include "sslmanagerbase.h"

void SslManager::ListenToServer() {
    qDebug() << "Listening to server";
    boost::asio::async_read(*socket_,
        boost::asio::buffer(recvbuf_, sizeof(S2CHeader)),
        boost::bind(&SslManager::HandleS2CHeader, this, boost::asio::placeholders::error));
    //run_io_service();
}
void SslManager::HandleS2CHeader(const boost::system::error_code& error) {
    HANDLE_ERROR;
    struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(recvbuf_);
    if (0 == s2cHeader->tsid) { //push
        switch (s2cHeader->type) {
        case S2C::INFO:
            boost::asio::async_read(*socket_,
                boost::asio::buffer(recvbuf_, sizeof(uint64_t)),
                boost::bind(&SslManager::HandleInfoHeader, this, boost::asio::placeholders::error));
            break;
        case S2C::FRIENDS:
            boost::asio::async_read(*socket_,
                boost::asio::buffer(recvbuf_, sizeof(uint64_t)),
                boost::bind(&SslManager::HandleFriendsHeader, this, boost::asio::placeholders::error));
            break;
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
        case S2C::DELETE_FRIEND:
            boost::asio::async_read(*socket_,
                boost::asio::buffer(recvbuf_, sizeof(userid_t)),
                boost::bind(&SslManager::HandleBeDeletedFriend, this, boost::asio::placeholders::error));
            break;
        case S2C::MSG:
            boost::asio::async_read(*socket_,
                boost::asio::buffer(recvbuf_, sizeof(MsgS2CHeader)),
                boost::bind(&SslManager::HandlePrivateMsgHeader, this, boost::asio::placeholders::error));
            break;
        case S2C::JOIN_GROUP_OK:
            boost::asio::async_read(*socket_,
                boost::asio::buffer(recvbuf_, sizeof(grpid_t)),
                boost::bind(&SslManager::HandleJoinGroupReply, this, boost::asio::placeholders::error));
            break;
        case S2C::GRPMSG:
            boost::asio::async_read(*socket_,
                boost::asio::buffer(recvbuf_, sizeof(S2CMsgGrpHeader)),
                boost::bind(&SslManager::HandleGrpMsgHeader, this, boost::asio::placeholders::error));
            break;
        case S2C::MOMENTS:
            boost::asio::async_read(*socket_,
                boost::asio::buffer(recvbuf_, sizeof(uint64_t)),
                boost::bind(&SslManager::HandleMomentsMainHeader, this, boost::asio::placeholders::error));
            break;
        case S2C::COMMENTS:
            boost::asio::async_read(*socket_,
                boost::asio::buffer(recvbuf_, sizeof(CommentArrayHeader)),
                boost::bind(&SslManager::HandleCommentArrayHeader, this, boost::asio::placeholders::error));
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
        case C2S::USER_PRIVATE_INFO_REQ:
            boost::asio::async_read(*socket_,
                boost::asio::buffer(recvbuf_, sizeof(UserPrivateInfoHeader)),
                boost::bind(&SslManager::HandleUserPrivateInfoHeader, this, boost::asio::placeholders::error));
            break;
        case C2S::USER_PUBLIC_INFO_REQ:
            boost::asio::async_read(*socket_,
                boost::asio::buffer(recvbuf_ + sizeof(S2CHeader), sizeof(UserPublicInfoHeader)),
                boost::bind(&SslManager::HandleUserPublicInfoHeader, this, boost::asio::placeholders::error));
            break;
        case C2S::FIND_BY_USERNAME:
            boost::asio::async_read(*socket_,
                boost::asio::buffer(recvbuf_ + sizeof(S2CHeader), sizeof(uint64_t)),
                boost::bind(&SslManager::HandleFindByUsernameReplyHeader, this, boost::asio::placeholders::error));
            break;
        case C2S::ADD_FRIEND_REQ:
            HandleAddFriendResponse();
            break;
        case C2S::MSG:
            HandleSendToUserResp();
            break;
        case C2S::CREATE_GROUP:
            //HandleCreateGroupReply();
            boost::asio::async_read(*socket_,
                boost::asio::buffer(recvbuf_ + sizeof(S2CHeader), sizeof(CreateGroupReply)),
                boost::bind(&SslManager::HandleCreateGroupReply, this, boost::asio::placeholders::error));
            break;
        case C2S::GRP_INFO:
            boost::asio::async_read(*socket_,
                boost::asio::buffer(recvbuf_ + sizeof(S2CHeader), sizeof(uint64_t)),
                boost::bind(&SslManager::HandleGrpInfoHeader, this, boost::asio::placeholders::error));
            break;
        case C2S::GRPMSG:
            boost::asio::async_read(*socket_,
                boost::asio::buffer(recvbuf_ + sizeof(S2CHeader), sizeof(S2CGrpMsgReply)),
                boost::bind(&SslManager::HandleGrpMsgReply, this, boost::asio::placeholders::error));
            break;
        default:
            qWarning() << "Unexpected transaction id: " << s2cHeader->tsid;
            break;
        }
        transactionType_.erase(it);
    }
}
