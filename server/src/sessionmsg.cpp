#include "sessionbase.h"

void session::handle_msg(const boost::system::error_code& error) {
    static_assert(sizeof(C2SHeader) + sizeof(MsgS2CHeader) + MAX_CONTENT_LEN <= BUFSIZE);
    if (error) {
        reset();
        return;
    }
    struct MsgC2SHeader *msgC2SHeader = reinterpret_cast<struct MsgC2SHeader*>
        (buf_ + sizeof(C2SHeader) + sizeof(MsgS2CReply));
    if (msgC2SHeader->len > MAX_CONTENT_LEN) {
        IgnoreMsgContent(msgC2SHeader->len);
        SendType(S2C::MSG_TOO_LONG);
        return;
    }
    boost::asio::async_read(socket_,
        boost::asio::buffer(msgC2SHeader + 1, msgC2SHeader->len),
        boost::bind(&session::handle_msg_content, this,
            boost::asio::placeholders::error));
}
void session::IgnoreMsgContent(size_t len) {
    if (len) {
        //QUESTION: std::min(len, BUFSIZE) will cause a compile error. The reasion is unclear.
        size_t sz = std::min(len, sizeof(buf_));
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_, sz),
            boost::bind(&session::HandleIgnore, this, len - sz,
                boost::asio::placeholders::error));
    } else {
        listen_request();
    }
}
void session::HandleIgnore(size_t len, const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    IgnoreMsgContent(len);
}
void session::handle_msg_content(const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    struct C2SHeader *c2sHeader = reinterpret_cast<struct C2SHeader *>(buf_);
    struct MsgS2CReply *msgS2CReply = reinterpret_cast<struct MsgS2CReply *>(c2sHeader + 1);
    struct MsgC2SHeader *msgC2SHeader = reinterpret_cast<struct MsgC2SHeader*>(msgS2CReply + 1);
    uint8_t *content = reinterpret_cast<uint8_t *>(msgC2SHeader + 1);
    //Write to db
    using namespace std::chrono;
    SQLLEN msgidLen = sizeof(msgid_t);
    SQLLEN contentLen = msgC2SHeader->len;
    SQLRETURN retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_OUTPUT, SQL_C_UBIGINT, 
        SQL_BIGINT, 0, 0, &msgS2CReply->msgid, 0, &msgidLen);
    if (!SQL_SUCCEEDED(retcode)) {
        std::cerr << "Error: handle_msg_content: SQLBindParameter of msgid failed!\n";
        reset();
        return;
    }
    retcode = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_BINARY,
        MAX_CONTENT_LEN, 0, content, msgC2SHeader->len, &contentLen);
    if (!SQL_SUCCEEDED(retcode)) {
        std::cerr << "Error: handle_msg_content: SQLBindParameter of content failed!\n";
        reset();
        return;
    }
    msgS2CReply->time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    if (odbc_exec(std::cerr, ("{CALL insert_msg(?, " +
        std::to_string(msgS2CReply->time) + ',' +
        std::to_string(userid) + ',' +  //from
        std::to_string(msgC2SHeader->to) + ",?)}"
    ).c_str())) {
        reset();
        return;
    }
    //Response to the sender
    struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(buf_);
    s2cHeader->type = S2C::MSG_RESP;
    SendLater(buf_, sizeof(S2CHeader) + sizeof(MsgS2CReply));
    //Forward to the receiver
    auto it = user_session.find(msgC2SHeader->to);
    if (it != user_session.end()) {
        struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(buf_);
        struct MsgS2CHeader *msgS2CHeader = static_cast<struct MsgS2CHeader *>(msgS2CReply);
        s2cHeader->tsid = 0;    //push
        s2cHeader->type = S2C::MSG;
        msgS2CHeader->from = userid;
        it->second->SendLater(s2cHeader, sizeof(S2CHeader) + sizeof(MsgS2CHeader) + msgS2CHeader->len);
    }
    listen_request();
}
