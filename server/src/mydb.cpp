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
        "signuptime INT NOT NULL,"
        "username CHAR(100) NOT NULL,"
        "phone CHAR(28),"
        "salt BINARY(32) COMMENT '密码盐值',"
        "pw BINARY(32) COMMENT '密码的哈希值加盐再哈希的值'"
    ");");
    odbc_exec(err, "CREATE INDEX usernameindex ON user(username(100));");
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
        "PRIMARY KEY(user1, user2),"
        "FOREIGN KEY friends_user1_foreign(user1) REFERENCES user(userid),"
        "FOREIGN KEY friends_user2_foreign(user2) REFERENCES user(userid)"
    ");");
    odbc_exec(err, "CREATE TABLE msg ("
        "msgid BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,"
        "msgtime BIGINT NOT NULL COMMENT '服务器接收到消息的时间',"
        "sender BIGINT UNSIGNED NOT NULL COMMENT '发送方id',"
        "touser BIGINT UNSIGNED NOT NULL,"
        "content BLOB(2000) NOT NULL,"
        "FOREIGN KEY msg_sender_foreign(sender) REFERENCES user(userid),"
        "FOREIGN KEY msg_touser_foreign(touser) REFERENCES user(userid)"
    ");");
    odbc_exec(err, "CREATE VIEW msgcnt AS SELECT sender, count(*) cnt FROM msg GROUP BY sender;");
    odbc_exec(err, "CREATE PROCEDURE insert_msg(OUT msgid BIGINT UNSIGNED, IN msgtime BIGINT, IN sender BIGINT UNSIGNED, IN touser BIGINT UNSIGNED, IN content BLOB(2000))\n"
        "BEGIN\n"
            "INSERT INTO msg(msgtime, sender, touser, content) VALUES(msgtime, sender, touser, content);\n"
            "SET msgid = LAST_INSERT_ID();\n"
        "END");
    odbc_exec(err, "CREATE TABLE grp ("
        "grpid BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,"
        "createtime INT NOT NULL,"
        "creator BIGINT UNSIGNED NOT NULL,"
        "grpowner BIGINT UNSIGNED NOT NULL,"
        "grpname CHAR(100) NOT NULL,"
        "FOREIGN KEY grp_creator_foreign(creator) REFERENCES user(userid),"
        "FOREIGN KEY grp_grpowner_foreign(grpowner) REFERENCES user(userid)"
    ");");
    odbc_exec(err, "CREATE PROCEDURE insert_grp(OUT grpid BIGINT UNSIGNED, IN createtime INT, IN creator BIGINT UNSIGNED, IN grpowner BIGINT UNSIGNED, IN grpname CHAR(100))\n"
        "BEGIN\n"
            "INSERT INTO grp(createtime, creator, grpowner, grpname) VALUES(createtime, creator, grpowner, grpname);\n"
            "SET grpid = LAST_INSERT_ID();\n"
        "END");
    odbc_exec(err, "CREATE TABLE grpmember ("
        "grpid BIGINT UNSIGNED,"
        "userid BIGINT UNSIGNED,"
        "PRIMARY KEY(grpid, userid),"
        "FOREIGN KEY grpmember_grpid_foreign(grpid) REFERENCES grp(grpid),"
        "FOREIGN KEY grpmember_userid_foreign(userid) REFERENCES user(userid)"
    ");");
    odbc_exec(err, "CREATE TABLE grpmsg ("
    "    grpmsgid BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,"
    "    grpmsgtime BIGINT NOT NULL,"
    "    sender BIGINT UNSIGNED NOT NULL,"
    "    togrp BIGINT UNSIGNED NOT NULL,"
    "    content BLOB(2000) NOT NULL,"
    "    FOREIGN KEY grpmsg_sender_foreign(sender) REFERENCES user(userid),"
    "    FOREIGN KEY grpmsg_togrp_foreign(togrp) REFERENCES grp(grpid)"
    ");");
    odbc_exec(err, "CREATE PROCEDURE insert_grpmsg(OUT grpmsgid BIGINT UNSIGNED, IN grpmsgtime BIGINT, IN sender BIGINT UNSIGNED, IN togrp BIGINT UNSIGNED, IN content BLOB(2000))\n"
        "BEGIN\n"
            "INSERT INTO grpmsg(grpmsgtime, sender, togrp, content) VALUES(grpmsgtime, sender, togrp, content);\n"
            "SET grpmsgid = LAST_INSERT_ID();\n"
        "END");
    odbc_exec(err, "CREATE TABLE moment ("
    "    id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,"
    "    time BIGINT NOT NULL,"
    "    sender BIGINT UNSIGNED NOT NULL,"
    "    ori BIGINT UNSIGNED COMMENT '转发自。为NULL表示为原创',"
    "    content BLOB(2000) NOT NULL COMMENT '若为转发的，则表示转发时加上的内容',"
    "    FOREIGN KEY moment_sender_foreign(sender) REFERENCES user(userid),"
    "    FOREIGN KEY moment_ori_foreign(ori) REFERENCES moment(id)"
    ");");
    odbc_exec(err, "CREATE INDEX index_moment_sender ON moment(sender);");
    // odbc_exec(err, "CREATE PROCEDURE insert_moment(OUT id BIGINT UNSIGNED, IN time BIGINT, IN sender BIGINT UNSIGNED, IN ori BIGINT UNSIGNED, IN content BLOB(2000))\n"
    //     "BEGIN\n"
    //         "INSERT INTO moment(time, sender, ori, content) VALUES(time, sender, ori, content);\n"
    //         "SET id = LAST_INSERT_ID();\n"
    //     "END");
    odbc_exec(err, "CREATE TABLE comment ("
    "    id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,"
    "    time BIGINT NOT NULL,"
    "    sender BIGINT UNSIGNED NOT NULL,"
    "    tommt BIGINT UNSIGNED NOT NULL COMMENT '(to moment)朋友圈id',"
    "    reply BIGINT UNSIGNED,"
    "    content BLOB(2000) NOT NULL,"
    "    FOREIGN KEY sender_foreign(sender) REFERENCES user(userid),        "
    "    FOREIGN KEY reply_foreign(reply) REFERENCES comment(id)"
    ");");
    odbc_exec(err, "CREATE INDEX comment_sender_index ON comment(sender);");
    // odbc_exec(err, "CREATE PROCEDURE insert_comment(OUT id BIGINT UNSIGNED, IN time BIGINT, IN sender BIGINT UNSIGNED, IN reply BIGINT UNSIGNED, IN content BLOB(2000))\n"
    //     "BEGIN\n"
    //         "INSERT INTO comment(time, sender, reply, content) VALUES(time, sender, reply, content);\n"
    //         "SET id = LAST_INSERT_ID();\n"
    //     "END");
}
