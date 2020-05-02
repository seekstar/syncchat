#ifndef SESSION_H_
#define SESSION_H_

#include <queue>
//std::max
#include <algorithm>
#include <boost/bind.hpp>

#include "sslbase.h"
#include "types.h"

class session {
public:
    session(boost::asio::io_service& io_service,
        boost::asio::ssl::context& context);

    ssl_socket::lowest_layer_type& socket();

    void start();
    void SendLater(void *data, size_t len);

private:
    void SendNow(void *data, size_t len);
    void StartSend();
    void handle_send(const boost::system::error_code& error);
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
    void handle_msg(const boost::system::error_code& error);
    void IgnoreMsgContent(size_t len);
    void HandleIgnore(size_t len, const boost::system::error_code& error);
    void handle_msg_content(const boost::system::error_code& error);
    void send_msg_content(const boost::system::error_code& error);
    void reset();
    
    ssl_socket socket_;
    //ssl_socket* socket2_;
    bool busy;
    
    userid_t userid;

    //Need C++14
    constexpr static size_t BUFSIZE = std::max({
        sizeof(C2SHeader),
        sizeof(SignupHeader) + 2 * SHA256_DIGEST_LENGTH +
            sizeof(S2CHeader) + sizeof(SignupReply) +
             + MAX_USERNAME_LEN + MAX_PHONE_LEN,
        sizeof(LoginInfo) + 3 * SHA256_DIGEST_LENGTH,
        sizeof(S2CHeader) + sizeof(MsgS2CHeader) + MAX_CONTENT_LEN
    });
    uint8_t buf_[BUFSIZE];
    /*const static size_t SBUFSIZE = std::max({
        sizeof(S2CHeader) + sizeof(signupreply),
    });
    char sbuf_[SBUFSIZE];*/
    std::vector<uint8_t> sending, sendbuf;
};

#endif //SESSION_H_