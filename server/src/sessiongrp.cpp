#include "sessionbase.h"

#include "escape.h"

void session::HandleCreateGroupHeader(const boost::system::error_code& error) {
    HANDLE_ERROR;
    uint64_t *groupnameLen = reinterpret_cast<uint64_t *>(buf_ + sizeof(C2SHeader));
    boost::asio::async_read(socket_,
        boost::asio::buffer(groupnameLen + 1, *groupnameLen),
        boost::bind(&session::HandleCreateGroupName, this, boost::asio::placeholders::error));
}
void session::HandleCreateGroupName(const boost::system::error_code& error) {
    HANDLE_ERROR;
    uint64_t *groupnameLen = reinterpret_cast<uint64_t *>(buf_ + sizeof(C2SHeader));
    char *groupname = reinterpret_cast<char *>(groupnameLen + 1);
    using namespace std::chrono;
    daystamp_t time = duration_cast<days>(system_clock::now().time_since_epoch()).count();
    grpid_t groupid;
    SQLLEN groupidLen;
    SQLRETURN retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_OUTPUT, SQL_C_UBIGINT, SQL_BIGINT, 0, 0, &groupid, sizeof(groupid), &groupidLen);
    if (!SQL_SUCCEEDED(retcode)) {
        dbgcout << "Error in " << __PRETTY_FUNCTION__ << ": SQLBindParameter of groupid failed\n";
        reset();
        return;
    }
    std::string stmt = "{CALL insert_grp(?, " +
        std::to_string(time) + ',' +
        std::to_string(userid) + ',' +
        std::to_string(userid) + ',' +
        '"' + escape(std::string(groupname, *groupnameLen)) + "\")}";
    if (odbc_exec(std::cerr, stmt.c_str())) {
        reset();
        //listen_signup_or_login();
        return;
    }

    auto s2cHeader = reinterpret_cast<struct S2CHeader *>(buf_);
    auto createGroupReply = reinterpret_cast<struct CreateGroupReply *>(s2cHeader + 1);
    //tsid is already in place
    s2cHeader->type = S2C::CREATE_GROUP_RESP;
    createGroupReply->grpid = groupid;
    createGroupReply->time = time;
    SendLater(buf_, sizeof(S2CHeader) + sizeof(CreateGroupReply));
    listen_request();
}
void session::HandleJoinGroup(const boost::system::error_code& error) {
    HANDLE_ERROR;
    grpid_t grpid = *reinterpret_cast<grpid_t *>(buf_);
    if (odbc_exec(std::cerr, ("INSERT INTO grpmember(grpid, userid) VALUES(" + std::to_string(grpid) + ',' +
        std::to_string(userid) + ");"
    ).c_str())) {
        //TODO: send ALREADY_GROUP_MEMBER
        dbgcout << "Already a member of group " << grpid << ", ignore it\n";
        listen_request();
        return;
    }

    S2CHeader *s2cHeader = reinterpret_cast<S2CHeader *>(buf_);
    s2cHeader->tsid = 0;    //push
    s2cHeader->type = S2C::JOIN_GROUP_OK;
    *reinterpret_cast<grpid_t *>(s2cHeader + 1) = grpid;
    SendLater(buf_, sizeof(S2CHeader) + sizeof(grpid_t));
    listen_request();
}

void session::HandleGrpInfoReq(const boost::system::error_code& error) {
    HANDLE_ERROR;
    auto s2cHeader = reinterpret_cast<S2CHeader *>(buf_);
    grpid_t grpid = *reinterpret_cast<grpid_t *>(s2cHeader + 1);
    if (odbc_exec(std::cerr, (
        "SELECT grpname FROM grp WHERE grpid = " + std::to_string(grpid) + ';'
    ).c_str())) {
        dbgcout << __PRETTY_FUNCTION__ << ": odbc_exec error\n";
        reset();
        return;
    }
    uint64_t *grpnameLen = reinterpret_cast<uint64_t *>(s2cHeader + 1);
    char *grpname = reinterpret_cast<char *>(grpnameLen + 1);
    SQLLEN length;
    SQLBindCol(hstmt, 1, SQL_C_CHAR, grpname, MAX_GROUPNAME_LEN, &length);
    if (SQL_NO_DATA == SQLFetch(hstmt)) {
        //TODO:
        dbgcout << __PRETTY_FUNCTION__ << ": no such a group " << grpid << '\n';
        reset();
        return;
    }
    odbc_close_cursor(std::cerr);
    if (SQL_NULL_DATA == length) {
        dbgcout << __PRETTY_FUNCTION__ << ": null grpname\n";
        length = 0;
    }
    *grpnameLen = length;
    //tsid is already in place
    s2cHeader->type = S2C::GRP_INFO;
    SendLater(buf_, sizeof(S2CHeader) + sizeof(uint64_t) + length);
}
