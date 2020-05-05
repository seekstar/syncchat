#include "mydb.h"

#include "odbcbase.h"
#include "cppbase.h"

void InitDB(std::ostream& err) {
    if (!odbc_exec(err, "USE wechat_SZ170110227;")) {
        return;
    }
    odbc_exec(err, "CREATE DATABASE wechat_SZ170110227;");
    odbc_exec(err, "USE wechat_SZ170110227;");
    odbc_exec(err, "CREATE TABLE user ("
        "userid BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,"
        "signuptime INT,"
        "username CHAR(100),"
        "phone CHAR(28),"
        "salt BINARY(32) COMMENT '密码盐值',"
        "pw BINARY(32) COMMENT '密码的哈希值加盐再哈希的值'"
    ");");
    odbc_exec(err,
        "CREATE PROCEDURE insert_user(OUT userid BIGINT UNSIGNED, IN signuptime INT, IN username CHAR(100), IN phone CHAR(28), salt BINARY(32), pw BINARY(32))\n"
        "BEGIN\n"
            "INSERT INTO user(signuptime, username, phone, salt, pw)\n"
            "VALUES(signuptime, username, phone, salt, pw);\n"
            "SET userid = LAST_INSERT_ID();\n"
        "END");
    odbc_exec(err, "CREATE TABLE friends ("
        "user1 BIGINT UNSIGNED,"
        "user2 BIGINT UNSIGNED,"
        "PRIMARY KEY(user1, user2)"
    ");");
    odbc_exec(err, "CREATE TABLE msg ("
        "msgid BIGINT UNSIGNED PRIMARY KEY,"
        "time BIGINT COMMENT '服务器接收到消息的时间',"
        "sender BIGINT UNSIGNED COMMENT '发送方id',"
        "touser BIGINT UNSIGNED,"
        "content BLOB(2000)"
    ");");
    odbc_exec(err, "CREATE TABLE gmsg ("
        "msgid BIGINT UNSIGNED PRIMARY KEY,"
        "time BIGINT COMMENT '服务器接收到消息的时间',"
        "sender BIGINT UNSIGNED COMMENT '发送方id',"
        "togroup BIGINT UNSIGNED,"
        "content BLOB(2000)"
    ");");
}