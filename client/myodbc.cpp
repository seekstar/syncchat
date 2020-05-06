#include "myodbc.h"

#include <QMessageBox>
#include <QDebug>

#include <sstream>

#include "odbcbase.h"

bool myodbcLogin(const char *connStr) {
    std::ostringstream err;
    if (odbc_driver_connect(err, connStr)) {
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
             "msgtime BIGINT,"
             "sender BIGINT UNSIGNED,"
             "touser BIGINT UNSIGNED,"
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
