#include "sessionbase.h"

#include "myrandom.h"
#include "escape.h"

void session::listen_signup_or_login() {
    dbgcout << "listen_signup_or_login\n";
    boost::asio::async_read(socket_,
        boost::asio::buffer(buf_, sizeof(C2SHeader)),
        boost::bind(&session::handle_signup_or_login, this, boost::asio::placeholders::error));
}

void session::listen_signup_or_login(const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    listen_signup_or_login();
}
void session::handle_signup_or_login(const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    dbgcout << "handle_signup_or_login\n";
    struct C2SHeader *header = reinterpret_cast<struct C2SHeader *>(buf_);
    switch (header->type) {
    case C2S::IMALIVE:
        dbgcout << "client alive\n";
        break;
    case C2S::SIGNUP:
        dbgcout << "signup request\n";
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_, sizeof(SignupHeader)),
            boost::bind(&session::HandleSignupHeader, this, 
                header->tsid, boost::asio::placeholders::error));
        break;
    case C2S::LOGIN:
        dbgcout << "login request\n";
        boost::asio::async_read(socket_,
            boost::asio::buffer(buf_, sizeof(LoginInfo)),
            boost::bind(&session::handle_login, this, header->tsid,
                boost::asio::placeholders::error));
        break;
    default:
        dbgcout << "handle_signup_or_login: Unknown type " << *(C2SBaseType*)buf_ << std::endl;
        break;
    }
}
void session::HandleSignupHeader(transactionid_t tsid, const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    
    struct SignupHeader *header = reinterpret_cast<struct SignupHeader *>(buf_);
    if (header->namelen > MAX_USERNAME_LEN) {
        SendType(tsid, S2C::USERNAME_TOO_LONG);
        listen_signup_or_login();
        return;
    }
    if (header->phonelen > MAX_PHONE_LEN) {
        SendType(tsid, S2C::PHONE_TOO_LONG);
        listen_signup_or_login();
        return;
    }

    boost::asio::async_read(socket_,
        boost::asio::buffer(
            buf_ + sizeof(SignupHeader) + 2 * SHA256_DIGEST_LENGTH + sizeof(S2CHeader) + sizeof(SignupReply), 
            header->namelen + header->phonelen),
        boost::bind(&session::HandleSignup, this, tsid, boost::asio::placeholders::error));
}
void session::HandleSignup(transactionid_t tsid, const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    struct SignupHeader *header = reinterpret_cast<struct SignupHeader *>(buf_);
    uint8_t *salt = buf_ + sizeof(SignupHeader);
    uint8_t *res = salt + SHA256_DIGEST_LENGTH;
    struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(res + SHA256_DIGEST_LENGTH);
    struct SignupReply *signupreply = reinterpret_cast<struct SignupReply *>(s2cHeader + 1);
    char *name = reinterpret_cast<char*>(signupreply + 1);
    char *phone = name + header->namelen;

    genrand(salt, SHA256_DIGEST_LENGTH);
    SHA256(header->pwsha256, 2 * SHA256_DIGEST_LENGTH, res);
    dbgcout << "sha256 of password: \n";
    DbgPrintHex(header->pwsha256, SHA256_DIGEST_LENGTH);
    dbgcout << "\nsalt generated is: \n";
    DbgPrintHex(salt, SHA256_DIGEST_LENGTH);
    dbgcout << "\nsha256(pwsha256, salt) is: \n";
    DbgPrintHex(res, SHA256_DIGEST_LENGTH);
    dbgcout << '\n';

    using namespace std::chrono;
    s2cHeader->tsid = tsid;
    s2cHeader->type = S2C::SIGNUP_RESP;
    signupreply->day = duration_cast<days>(system_clock::now().time_since_epoch()).count();

    SQLLEN length;
    SQLRETURN retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_OUTPUT, SQL_C_UBIGINT,
        SQL_BIGINT, 0, 0, &signupreply->id, 0, &length);
    if (!SQL_SUCCEEDED(retcode)) {
        std::cerr << "HandleSignup: SQLBindParameter of user id Failed" << std::endl;
        //listen_signup_or_login();
        reset();
        return;
    }
    length = SHA256_DIGEST_LENGTH;
    retcode = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_BINARY,
        SQL_BINARY, SHA256_DIGEST_LENGTH, 0, salt, SHA256_DIGEST_LENGTH, &length);
    if (!SQL_SUCCEEDED(retcode)) {
        std::cerr << "HandleSignup: SQLBindParameter of user salt Failed" << std::endl;
        //listen_signup_or_login();
        reset();
        return;
    } 
    length = SHA256_DIGEST_LENGTH;
    retcode = SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_BINARY,
        SQL_BINARY, SHA256_DIGEST_LENGTH, 0, res, SHA256_DIGEST_LENGTH, &length);
    if (!SQL_SUCCEEDED(retcode)) {
        std::cerr << "HandleSignup: SQLBindParameter of user pw Failed" << std::endl;
        //listen_signup_or_login();
        reset();
        return;
    }
    std::string stmt = "{CALL insert_user(?, " +
        std::to_string(signupreply->day) + ',' +
        '"' + escape(name, header->namelen) + "\"," +
        '"' + escape(phone, header->phonelen) + "\"," +
        "?, ?)}";
    if (odbc_exec(std::cerr, stmt.c_str())) {
        reset();
        //listen_signup_or_login();
        return;
    }
    dbgcout << "Signup ok. id = " << signupreply->id << '\n';
    SendLater(s2cHeader, sizeof(S2CHeader) + sizeof(SignupReply));
    listen_signup_or_login();
    // boost::asio::async_write(socket_,
    //     boost::asio::buffer(s2cHeader, sizeof(S2CHeader) + sizeof(SignupReply)),
    //     boost::bind(&session::listen_signup_or_login, this, boost::asio::placeholders::error));
}
void session::handle_login(transactionid_t tsid, const boost::system::error_code& error) {
    if (error) {
        reset();
        return;
    }
    struct LoginInfo *loginInfo = reinterpret_cast<struct LoginInfo *>(buf_);
    uint8_t *salt = buf_ + sizeof(LoginInfo);
    uint8_t *pw = salt + SHA256_DIGEST_LENGTH;
    uint8_t *res = pw + SHA256_DIGEST_LENGTH;

    std::string stmt("SELECT salt, pw FROM user WHERE userid = " + 
        std::to_string(loginInfo->userid) + ';');
    if (odbc_exec(std::cerr, stmt.c_str())
    ) {
        std::cerr << "Error: handle_login: odbc_exec " << stmt << '\n';
        reset();
        return;
    }
    SQLLEN length;
    SQLBindCol(hstmt, 1, SQL_C_BINARY, salt, SHA256_DIGEST_LENGTH, &length);
    SQLBindCol(hstmt, 2, SQL_C_BINARY, pw, SHA256_DIGEST_LENGTH, &length);
    if (SQL_NO_DATA == SQLFetch(hstmt)) {
        dbgcout << "NO_SUCH_USER\n";
        SendType(tsid, S2C::NO_SUCH_USER);
        listen_signup_or_login();
        return;
    }
    if (odbc_close_cursor(std::cerr)) {
        reset();
        return;
    }
    SHA256(loginInfo->pwsha256, 2 * SHA256_DIGEST_LENGTH, res);
    dbgcout << "Given sha256 of password is: \n";
    DbgPrintHex(loginInfo->pwsha256, SHA256_DIGEST_LENGTH);
    dbgcout << "\nstored salt is: \n";
    DbgPrintHex(salt, SHA256_DIGEST_LENGTH);
    dbgcout << "\nsha256(pwsha256, salt) = \n";
    DbgPrintHex(res, SHA256_DIGEST_LENGTH);
    dbgcout << "\nThe stored result is: \n";
    DbgPrintHex(pw, SHA256_DIGEST_LENGTH);
    dbgcout << '\n';
    if (memcmp(pw, res, SHA256_DIGEST_LENGTH)) {
        dbgcout << "WRONG_PASSWORD\n";
        SendType(tsid, S2C::WRONG_PASSWORD);
        listen_signup_or_login();
        return;
    }
    dbgcout << "LOGIN_OK\n";
    userid = loginInfo->userid;
    user_session[userid] = this;
    dbgcout << "user " << userid << " online now\n";
    SendType(tsid, S2C::LOGIN_OK);
    listen_request();
}
