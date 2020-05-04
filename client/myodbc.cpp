#include "myodbc.h"

#include <QMessageBox>
#include <QDebug>

#include <sstream>

#include "odbcbase.h"

bool myodbcLogin(void) {
    std::ostringstream err;
    if (odbc_driver_connect(err, "Driver=SQLite3;Database=syncchatclient.db")) {
        QMessageBox::critical(NULL, "Can not open syncchatclient.db", err.str().c_str());
        return true;
    }
    /*exec_sql("CREATE DATABASE syncchat;", false);
    if (exec_sql("USE syncchat;", true)) {
        return true;
    }*/
    exec_sql("CREATE TABLE friends ("
             "userid BIGINT UNSIGNED PRIMARY KEY,"
             "username CHAR(100)"
             ");", false);
    exec_sql("CREATE TABLE msg ("
             "msgid BIGINT UNSIGNED PRIMARY KEY,"
             "time BIGINT COMMENT '服务器接收到消息的时间',"
             "sender BIGINT UNSIGNED COMMENT '发送方id',"
             "content BLOB(2000)"
             ");", false);
    return false;
}

bool myodbcLogout(void) {
    if (SQL_NULL_HSTMT == hstmt) {
        return false;
    }
    std::ostringstream info;
    if (odbc_free_all(info)) {
        QMessageBox::critical(NULL, "odbc logout error", QString(info.str().c_str()));
        return true;
    }
    return false;
}

bool exec_sql(const std::string& stmt, bool critical) {
    qDebug() << stmt.c_str();
    std::ostringstream err;
    if (odbc_exec(err, stmt.c_str())) {
        if (critical) {
            qDebug() << "ERROR!";
            QMessageBox::critical(NULL, "Execute error!", QString(err.str().c_str()));
        } else {
            qDebug() << "Uncritical execute error:" << err.str().c_str();
        }
        return true;
    }
    return false;
}
