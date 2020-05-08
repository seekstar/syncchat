#include "sessionbase.h"

void session::HandleGrpMsg(const boost::system::error_code& error) {
    dbgcout << __PRETTY_FUNCTION__;
    HANDLE_ERROR;
    auto c2sGrpMsgHeader = reinterpret_cast<struct C2SGrpMsgHeader*>
        (buf_ + sizeof(C2SHeader) + sizeof(S2CGrpMsgReply) + sizeof(userid_t));
    if (c2sGrpMsgHeader->len > MAX_CONTENT_LEN) {
        //TODO: Handle it
        //IgnoreMsgContent(c2sGrpMsgHeader->len);
        //SendType(S2C::MSG_TOO_LONG);
        reset();
        return;
    }
    boost::asio::async_read(socket_,
        boost::asio::buffer(c2sGrpMsgHeader + 1, c2sGrpMsgHeader->len),
        boost::bind(&session::HandleGrpMsgContent, this,
            boost::asio::placeholders::error));
}
void session::HandleGrpMsgContent(const boost::system::error_code& error) {
    dbgcout << __PRETTY_FUNCTION__;
    HANDLE_ERROR;
    struct C2SHeader *c2sHeader = reinterpret_cast<struct C2SHeader *>(buf_);
    auto s2cGrpMsgReply = reinterpret_cast<struct S2CGrpMsgReply *>(c2sHeader + 1);
    userid_t *sender = reinterpret_cast<userid_t *>(s2cGrpMsgReply + 1);
    auto c2sGrpMsgHeader = reinterpret_cast<struct C2SGrpMsgHeader*>(sender + 1);
    uint8_t *content = reinterpret_cast<uint8_t *>(c2sGrpMsgHeader + 1);
    //Write to db
    using namespace std::chrono;
    SQLLEN grpmsgidLen = sizeof(grpmsgid_t);
    SQLLEN contentLen = c2sGrpMsgHeader->len;
    SQLRETURN retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_OUTPUT, SQL_C_UBIGINT, 
        SQL_BIGINT, 0, 0, &s2cGrpMsgReply->grpmsgid, 0, &grpmsgidLen);
    if (!SQL_SUCCEEDED(retcode)) {
        std::cerr << "Error in " << __PRETTY_FUNCTION__ << ": SQLBindParameter of grpmsgid failed!\n";
        reset();
        return;
    }
    retcode = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_BINARY,
        MAX_CONTENT_LEN, 0, content, c2sGrpMsgHeader->len, &contentLen);
    if (!SQL_SUCCEEDED(retcode)) {
        std::cerr << "Error in " << __PRETTY_FUNCTION__ << ": SQLBindParameter of content failed!\n";
        reset();
        return;
    }
    s2cGrpMsgReply->time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    if (odbc_exec(std::cerr, ("{CALL insert_grpmsg(?, " +
        std::to_string(s2cGrpMsgReply->time) + ',' +
        std::to_string(userid) + ',' +  //from
        std::to_string(c2sGrpMsgHeader->to) + ",?)}"
    ).c_str())) {
        reset();
        return;
    }
    dbgcout << __PRETTY_FUNCTION__ << ": generated message id is " << s2cGrpMsgReply->grpmsgid << '\n';

    //Response to the sender
    struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(buf_);
    s2cHeader->type = S2C::GRPMSG_RESP;
    SendLater(buf_, sizeof(S2CHeader) + sizeof(S2CGrpMsgReply));
    //Forward to the receiver
    auto s2cMsgGrpHeader = static_cast<struct S2CMsgGrpHeader *>(s2cGrpMsgReply);
    s2cHeader->tsid = 0;    //push
    s2cHeader->type = S2C::GRPMSG;
    s2cMsgGrpHeader->from = userid;
    if (odbc_exec(std::cerr, (
        "SELECT userid FROM grpmember WHERE grpid = " + std::to_string(s2cMsgGrpHeader->to) + ';'
    ).c_str())) {
        reset();
        return;
    }
    userid_t touser;
    SQLLEN length;
    SQLBindCol(hstmt, 1, SQL_C_UBIGINT, &touser, sizeof(touser), &length);
    while (SQL_NO_DATA != SQLFetch(hstmt)) {
        //TODO: Handle offline case
        if (touser == userid)
            continue;
        auto it = user_session.find(touser);
        if (it != user_session.end()) {
            it->second->SendLater(s2cHeader, sizeof(S2CHeader) + sizeof(S2CMsgGrpHeader) + s2cMsgGrpHeader->len);
        }
    }
    listen_request();
}
