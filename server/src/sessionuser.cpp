#include "sessionbase.h"

void session::HandleUserPrivateInfoReq() {
    dbgcout << __PRETTY_FUNCTION__ << '\n';
    auto s2cHeader = reinterpret_cast<struct S2CHeader *>(buf_);
    auto userPrivateInfoHeader = reinterpret_cast<struct UserPrivateInfoHeader *>(s2cHeader + 1);
    char *username = reinterpret_cast<char *>(userPrivateInfoHeader + 1);
    char *phone = reinterpret_cast<char *>(username + MAX_USERNAME_LEN);
    //tsid is already in place
    s2cHeader->type = S2C::USER_PRIVATE_INFO;
    SQLLEN nameLen, phoneLen;
    if (odbc_exec(std::cerr, (
        "SELECT username, phone FROM user WHERE userid = " + std::to_string(userid) + ';'
    ).c_str())) {
        std::cerr << "Error in " << __PRETTY_FUNCTION__ << ": odbc_exec fail\n";
        reset();
        return;
    }
    SQLBindCol(hstmt, 1, SQL_C_CHAR, username, MAX_USERNAME_LEN, &nameLen);
    SQLBindCol(hstmt, 2, SQL_C_CHAR, phone, MAX_PHONE_LEN, &phoneLen);
    if (SQL_NO_DATA == SQLFetch(hstmt)) {
        std::cerr << "Error in " << __PRETTY_FUNCTION__ << ": the user logined disappeared in db: " << userid << '\n';
        reset();
        return;
    }
    odbc_close_cursor(std::cerr);
    if (SQL_NULL_DATA == nameLen) {
        nameLen = 0;
    }
    if (SQL_NULL_DATA == phoneLen) {
        phoneLen = 0;
    }
    userPrivateInfoHeader->nameLen = nameLen;
    userPrivateInfoHeader->phoneLen = phoneLen;
    memcpy(username + nameLen, phone, phoneLen);
    SendLater(buf_, sizeof(S2CHeader) + sizeof(UserPrivateInfoHeader) + nameLen + phoneLen);
    listen_request();
}

void session::HandleUserPublicInfoReq(const boost::system::error_code& error) {
    dbgcout << __PRETTY_FUNCTION__ << '\n';
    HANDLE_ERROR;
    S2CHeader *s2cHeader = reinterpret_cast<S2CHeader *>(buf_);
    userid_t *id = reinterpret_cast<userid_t *>(s2cHeader + 1);
    UserPublicInfoHeader *publicInfo = reinterpret_cast<UserPublicInfoHeader *>(id);
    char *username = reinterpret_cast<char *>(publicInfo + 1);

    if (odbc_exec(std::cerr, 
        ("SELECT username FROM user WHERE userid = " + std::to_string(*id)).c_str()
    )) {
        reset();
        return;
    }
    SQLLEN length;
    SQLBindCol(hstmt, 1, SQL_C_CHAR, username, MAX_USERNAME_LEN, &length);
    //TODO: Handle error case
    if (SQL_NO_DATA == SQLFetch(hstmt)) {
        SendType(S2C::NO_SUCH_USER);
        listen_request();
        return;
    }
    if (odbc_close_cursor(std::cerr)) {
        reset();
        return;
    }
    //s2cHeader->tsid is already in place
    s2cHeader->type = S2C::USER_PUBLIC_INFO;
    assert(SQL_NULL_DATA != length);
    publicInfo->nameLen = length;
    SendLater(buf_, sizeof(S2CHeader) + sizeof(UserPublicInfoHeader) + length);
    listen_request();
}
