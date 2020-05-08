#ifndef SESSION_H_
#define SESSION_H_

#include <queue>
//std::max
#include <algorithm>
#include <boost/bind.hpp>

#include "sslbase.h"
#include "types.h"
#include "sslio.h"

class session : public SslIO {
public:
    session(boost::asio::io_service& io_service,
        boost::asio::ssl::context& context);

    ssl_socket::lowest_layer_type& socket();

    void start();

private:
    void handle_sslio_error(const boost::system::error_code& error);
    void SendType(transactionid_t tsid, S2C type);
    void SendType(S2C type);

    void listen_signup_or_login();
    void listen_signup_or_login(const boost::system::error_code& error);
    void handle_signup_or_login(const boost::system::error_code& error);
    void HandleSignupHeader(transactionid_t tsid, const boost::system::error_code& error);
    void HandleSignup(transactionid_t tsid, const boost::system::error_code& error);
    void handle_login(transactionid_t tsid, const boost::system::error_code& error);

    void listen_request();
    void listen_request(const boost::system::error_code& error);
    void handle_request(const boost::system::error_code& error);
    
    void HandleUserPrivateInfoReq();
    void HandleUserPublicInfoReq(const boost::system::error_code& error);
    void HandleFindByUsernameHeader(const boost::system::error_code& error);
    void HandleFindByUsernameContent(const boost::system::error_code& error);
    void HandleAddFriendReq(const boost::system::error_code& error);
    void HandleAddFriendReply(const boost::system::error_code& error);
    void HandleDeleteFriend(const boost::system::error_code& error);
    void HandleAllFriendsReq();

    void handle_msg(const boost::system::error_code& error);
    void IgnoreMsgContent(size_t len);
    void HandleIgnore(size_t len, const boost::system::error_code& error);
    void handle_msg_content(const boost::system::error_code& error);
    void send_msg_content(const boost::system::error_code& error);

    void HandleCreateGroupHeader(const boost::system::error_code& error);
    void HandleCreateGroupName(const boost::system::error_code& error);
    bool AddGroupMember(grpid_t grpid, userid_t userid);
    void HandleJoinGroup(const boost::system::error_code& error);
    void HandleGrpInfoReq(const boost::system::error_code& error);
    void HandleGrpMsg(const boost::system::error_code& error);
    void HandleGrpMsgContent(const boost::system::error_code& error);

    void reset();
    
    userid_t userid;
    //std::string username;
    //std::string userphone;

    //Need C++14
    constexpr static size_t BUFSIZE = std::max({
        sizeof(C2SHeader),
        sizeof(SignupHeader) + 2 * SHA256_DIGEST_LENGTH +
            sizeof(S2CHeader) + sizeof(SignupReply) +
             + MAX_USERNAME_LEN + MAX_PHONE_LEN,
        sizeof(LoginInfo) + 3 * SHA256_DIGEST_LENGTH,
        sizeof(C2SHeader) + sizeof(userid_t),   //HandleUserPublicInfoReq
        sizeof(S2CHeader) + sizeof(UserPrivateInfoHeader) + MAX_USERNAME_LEN + MAX_PHONE_LEN,
        sizeof(S2CHeader) + sizeof(UserPublicInfoHeader) + MAX_USERNAME_LEN,
        sizeof(C2SHeader) + sizeof(C2SAddFriendReq), 
        sizeof(S2CHeader) + sizeof(S2CAddFriendReqHeader) + MAX_USERNAME_LEN,
        sizeof(S2CHeader) + sizeof(S2CAddFriendReply),
        sizeof(S2CHeader) + sizeof(uint64_t) + sizeof(userid_t),    //At least one place to store friend id
        sizeof(S2CHeader) + sizeof(MsgS2CHeader) + MAX_CONTENT_LEN,
        sizeof(C2SHeader) + sizeof(uint64_t) + MAX_GROUPNAME_LEN,
        sizeof(S2CHeader) + sizeof(CreateGroupReply),
        sizeof(C2SHeader) + sizeof(grpid_t), sizeof(S2CHeader) + sizeof(grpid_t), //Join group
        sizeof(S2CHeader) + sizeof(uint64_t) + MAX_GROUPNAME_LEN,       //GRP_INFO
        sizeof(S2CHeader) + sizeof(S2CMsgGrpHeader) + MAX_CONTENT_LEN
    });
    uint8_t buf_[BUFSIZE];
    /*const static size_t SBUFSIZE = std::max({
        sizeof(S2CHeader) + sizeof(signupreply),
    });
    char sbuf_[SBUFSIZE];*/
};

#endif //SESSION_H_