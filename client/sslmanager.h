#ifndef SSLMANAGER_H
#define SSLMANAGER_H

#include <QThread>
#include <QTimer>

#include <unordered_map>

#include "sslbase.h"
#include "types.h"
#include "clienttypes.h"

class SslManager : public QThread
{
    Q_OBJECT
protected:
    void run();

public:
    explicit SslManager(const char *ip, const char *port);
    ~SslManager();
    //void stop_io_service();

signals:
    void sigErr(std::string);
    void sigDone();
    void loginFirst();
    void alreadyLogined();
    //void alreadyLogouted();
    void usernameTooLong();
    void phoneTooLong();
    void signupDone(userid_t userid);
    void noSuchUser();
    void wrongPassword();
    void loginDone(userid_t userid);

    void info(std::string content);
    void UserPrivateInfoReply(std::string username, std::string phone);
    void UserPublicInfoReply(userid_t userid, std::string username);
    void FindByUsernameReply(std::vector<userid_t>);

    void Friends(std::vector<userid_t> friends);
    void alreadyFriends();
    void addFriendSent();
    void addFriendReq(userid_t userid, std::string username);
    void addFriendReply(userid_t userid, bool reply);
    void BeDeletedFriend(userid_t userid);
    void PrivateMsgTooLong(userid_t userid, msgcontent_t content);
    void PrivateMsgResponse(userid_t userid, msgcontent_t content, msgid_t msgid, msgtime_t msgtime);
    void PrivateMsg(userid_t userid, msgcontent_t content, msgid_t msgid, msgtime_t msgtime);
    void NewGroup(grpid_t grpid, std::string grpname);
    void GrpMsgResp(grpmsgid_t grpmsgid, msgtime_t msgtime, grpid_t grpid, msgcontent_t content);
    void GrpMsg(grpmsgid_t grpmsgid, msgtime_t msgtime, userid_t sender, grpid_t grpid, msgcontent_t content);

public slots:
    void sslconn();
    void signup(std::vector<uint8_t> content);
    //void login(userid_t userid, uint8_t pwsha256[SHA256_DIGEST_LENGTH]);
    void login(struct LoginInfo loginInfo);
    void UserPrivateInfoReq();
    void UserPublicInfoReq(userid_t userid);

    void FindByUsername(std::string username);
    void AllFriendsReq();
    void AddFriend(userid_t userid);
    void ReplyAddFriend(userid_t userid, bool reply);
    void DeleteFriend(userid_t userid);
    void SendToUser(userid_t userid, msgcontent_t content);

    void CreateGroup(std::string groupname);
    void JoinGroup(grpid_t grpid);
    void GrpInfoReq(grpid_t grpid);
    void AllGrps();
    void AllGrpMember(grpid_t grpid);
    void SendToGroup(grpid_t grpid, msgcontent_t content);

    void SendMoment(msgcontent_t content);

private:
    //void run_io_service();
    bool verify_certificate(bool preverified,
                            boost::asio::ssl::verify_context& ctx);
    void handle_connect(const boost::system::error_code& error);
    void handle_handshake(const boost::system::error_code& error);

    void SendLater(const void *data, size_t len);
    void StartSend();
    void handle_send(const boost::system::error_code& error);
    std::vector<uint8_t> C2SHeaderBuf(C2S type);
    std::vector<uint8_t> C2SHeaderBuf_noreply(C2S type);
    void SendLater(const std::vector<uint8_t>& buf);

    void ListenToServer();
    void HandleS2CHeader(const boost::system::error_code& error);
    void HandleSignupReply();
    void HandleSignupReply2(const boost::system::error_code& error);
    void HandleLoginReply();

    void HandleInfoHeader(const boost::system::error_code& error);
    void HandleInfoContent(const boost::system::error_code& error);

    void HandleUserPrivateInfoHeader(const boost::system::error_code& error);
    void HandleUserPrivateInfoContent(const boost::system::error_code& error);
    void HandleUserPublicInfoHeader(const boost::system::error_code& error);
    void HandleUserPublicInfoContent(const boost::system::error_code& error);
    void HandleFindByUsernameReplyHeader(const boost::system::error_code& error);
    void HandleFindByUsernameReplyContent(const boost::system::error_code& error);

    void HandleFriendsHeader(const boost::system::error_code& error);
    void HandleFriendsContent(uint64_t num, const boost::system::error_code& error);
    void HandleFriendsContentNoError(uint64_t num);
    void HandleFriendsContentMore(uint64_t num, uint64_t readNum, const boost::system::error_code &error);

    void HandleAddFriendResponse();
    void HandleAddFriendReqHeader(const boost::system::error_code& error);
    void HandleAddFriendReqContent(const boost::system::error_code& error);
    void HandleAddFriendReply(const boost::system::error_code& error);
    void HandleBeDeletedFriend(const boost::system::error_code& error);

    void HandleSendToUserResp();
    void HandleSendToUserResp2(const boost::system::error_code& error, userid_t userid, msgcontent_t content);
    void HandlePrivateMsgHeader(const boost::system::error_code& error);
    void HandlePrivateMsgContent(const boost::system::error_code& error);

    void HandleCreateGroupReply(const boost::system::error_code& error);
    void HandleJoinGroupReply(const boost::system::error_code& error);
    void HandleGrpInfoHeader(const boost::system::error_code& error);
    void HandleGrpInfoContent(const boost::system::error_code& error);

    void HandleGrpMsgReply(const boost::system::error_code& error);
    void HandleGrpMsgHeader(const boost::system::error_code& error);
    void HandleGrpMsgContent(const boost::system::error_code &error);

    io_service_t io_service;
    //std::unique_ptr<boost::asio::io_service::work> work;
    QTimer timer;
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::ip::tcp::resolver::query  query;
    boost::asio::ip::tcp::resolver::iterator iterator;
    context_t *context;
    ssl_socket *socket_;
    constexpr static size_t RECVBUFSIZE = std::max({
        sizeof(SignupReply),
        sizeof(UserPrivateInfoHeader) + MAX_USERNAME_LEN + MAX_PHONE_LEN,
        sizeof(S2CHeader) + sizeof(UserPublicInfoHeader) + MAX_USERNAME_LEN,
        sizeof(S2CAddFriendReqHeader) + MAX_USERNAME_LEN,
        sizeof(S2CAddFriendReply),
        sizeof(uint64_t), sizeof(userid_t),   //FRIENDS: at least one place to hold one friend id
        sizeof(MsgS2CReply),
        sizeof(MsgS2CHeader) + MAX_CONTENT_LEN,
        sizeof(S2CHeader) + sizeof(CreateGroupReply),
        sizeof(S2CHeader) + sizeof(grpid_t),            //Join group reply
        sizeof(S2CHeader) + sizeof(uint64_t) + MAX_GROUPNAME_LEN    //group info
    });
    uint8_t recvbuf_[RECVBUFSIZE];
    bool busy;
    std::vector<uint8_t> sending, sendbuf;

    transactionid_t last_tsid;
    std::unordered_map<transactionid_t, C2S> transactionType_;
//    transactionid_t lastSignupTransaction_;
//    transactionid_t lastLoginTransaction_;
    typedef std::unordered_map<transactionid_t, userid_t> TransactionUserMap;
    TransactionUserMap transactionUser_;
    //std::unordered_map<transactionid_t, std::string> transactionUsername_;
    std::unordered_map<transactionid_t, msgcontent_t> transactionContent_;
    std::unordered_map<transactionid_t, grpid_t> transactionGrp_;
    std::unordered_map<transactionid_t, std::string> transactionGroupName_;
};

#endif // SSLMANAGER_H
