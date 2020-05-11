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
    if (AddGroupMember(groupid, userid)) {
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
bool session::AddGroupMember(grpid_t grpid, userid_t userid) {
    if (odbc_exec(std::cerr, ("INSERT INTO grpmember(grpid, userid) VALUES(" + std::to_string(grpid) + ',' +
        std::to_string(userid) + ");"
    ).c_str())) {
        //TODO: send ALREADY_GROUP_MEMBER
        dbgcout << "Already a member of group " << grpid << ", ignore it\n";
        listen_request();
        return true;
    }
    return false;
}
void session::HandleJoinGroup(const boost::system::error_code& error) {
    HANDLE_ERROR;
    grpid_t grpid = *reinterpret_cast<grpid_t *>(buf_);
    if (AddGroupMember(grpid, userid)) {
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
    listen_request();
}

void session::HandleChangeGrpOwner(const boost::system::error_code& error) {
    HANDLE_ERROR;
    grpid_t grpid = *reinterpret_cast<grpid_t *>(buf_);
    userid_t userid = *reinterpret_cast<userid_t *>(buf_ + sizeof(grpid_t));
    std::ostringstream err;
    if (odbc_exec(err, (
        "UPDATE grp SET grpowner = " + std::to_string(userid) + " WHERE grpid = " + std::to_string(grpid) + ';'
    ).c_str())) {
        SendInfo(err.str());
    }
    listen_request();
}

void session::HandleAllGrps() {
    if (odbc_exec(std::cerr, (
        "SELECT grpid FROM grpmember WHERE userid = " + std::to_string(userid) + ';'
    ).c_str())) {
        reset();
        return;
    }
    S2CHeader *s2cHeader = reinterpret_cast<S2CHeader *>(buf_);
    uint64_t *num = reinterpret_cast<uint64_t *>(s2cHeader + 1);
    grpid_t *grpidStart = reinterpret_cast<grpid_t *>(num + 1);
    s2cHeader->tsid = 0;    //push
    s2cHeader->type = S2C::GROUPS;

    *num = 0;
    grpid_t *grpid = grpidStart;
    while (1) {
        if (reinterpret_cast<uint8_t *>(grpid + 1) > buf_ + BUFSIZE) {
            SendLater(buf_, reinterpret_cast<uint8_t *>(grpid) - buf_);
            grpid = grpidStart;
            *num = 0;
        }
        SQLLEN length;
        SQLBindCol(hstmt, 1, SQL_C_UBIGINT, grpid, sizeof(grpid_t), &length);
        if (SQL_NO_DATA != SQLFetch(hstmt)) {
            ++*num;
            ++grpid;
        } else {
            if (*num) {
                SendLater(buf_, reinterpret_cast<uint8_t *>(grpid) - buf_);
            }
            break;
        }
    }
    listen_request();
}

void session::HandleAllGrpMember(const boost::system::error_code& error) {
    HANDLE_ERROR;
    grpid_t grpid = *reinterpret_cast<grpid_t *>(buf_);
    if (odbc_exec(std::cerr, (
        "SELECT userid, username FROM user WHERE userid in ("
        "SELECT userid FROM grpmember WHERE grpid = " + std::to_string(grpid) +
        ");"
    ).c_str())) {
        reset();
        return;
    }
    
    std::ostringstream disp;
    SQLLEN idLen, nameLen;
    userid_t id;
    char username[MAX_USERNAME_LEN];
    SQLBindCol(hstmt, 1, SQL_C_UBIGINT, &id, sizeof(id), &idLen);
    SQLBindCol(hstmt, 2, SQL_C_CHAR, username, sizeof(username), &nameLen);
    disp << "账号\t用户名\n";
    while (SQL_NO_DATA != SQLFetch(hstmt)) {
        disp << id << '\t' << std::string(username, nameLen) << '\n';
    }
    SendInfo(disp.str());
    listen_request();
}
