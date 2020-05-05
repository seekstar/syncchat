#ifndef SSLMANAGER_H
#define SSLMANAGER_H

#include <QThread>
#include <QTimer>

#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "sslbase.h"
#include "types.h"

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
    void wrongPassword();
    void loginDone();
    void UserPublicInfoReply(userid_t userid, std::string username);
    void alreadyFriends();
    void addFriendSent();
    void addFriendReply(userid_t userid, bool reply);
    void addFriendReq(userid_t userid, std::string username);

public slots:
    void sslconn();
    void signup(std::vector<uint8_t> content);
    //void login(userid_t userid, uint8_t pwsha256[SHA256_DIGEST_LENGTH]);
    void login(struct LoginInfo loginInfo);
    void UserPublicInfoReq(userid_t userid);
    void AddFriend(userid_t userid);

private:
    //void run_io_service();
    bool verify_certificate(bool preverified,
                            boost::asio::ssl::verify_context& ctx);
    void handle_connect(const boost::system::error_code& error);
    void handle_handshake(const boost::system::error_code& error);

    void SendLater(void *data, size_t len);
    void StartSend();
    void handle_send(const boost::system::error_code& error);
    std::vector<uint8_t> C2SHeaderBuf(C2S type);

    void ListenToServer();
    void HandleS2CHeader(const boost::system::error_code& error);
    void HandleSignupReply();
    void HandleSignupReply2(const boost::system::error_code& error);
    void HandleLoginReply();
    void HandleUserPublicInfo();
    void HandleAddFriendResponse();
    //void HandleAddFriendReply();
    void HandleAddFriendReply(const boost::system::error_code& error);

    io_service_t io_service;
    //std::unique_ptr<boost::asio::io_service::work> work;
    QTimer timer;
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::ip::tcp::resolver::query  query;
    boost::asio::ip::tcp::resolver::iterator iterator;
    context_t *context;
    ssl_socket *socket_;
    constexpr static size_t RECVBUFSIZE = std::max({
        sizeof(S2CHeader), sizeof(SignupReply)
    });
    uint8_t recvbuf_[RECVBUFSIZE];
    bool busy;
    std::vector<uint8_t> sending, sendbuf;

    transactionid_t last_tsid;
    std::unordered_map<transactionid_t, C2S> transactionType_;
//    transactionid_t lastSignupTransaction_;
//    transactionid_t lastLoginTransaction_;
};

#endif // SSLMANAGER_H
