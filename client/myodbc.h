#ifndef MYODBC_H
#define MYODBC_H

#include <string>

#include "sqlext_compatible.h"

extern SQLHSTMT hstmt;

bool myodbcLogin(const char *connStr);
bool myodbcLogout(void);
bool exec_sql(const std::string& stmt, bool critical);
bool close_cursor();

#endif // MYODBC_H
