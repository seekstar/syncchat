#ifndef MYODBC_H
#define MYODBC_H

#include <string>

bool myodbcLogin(const char *connStr);
bool myodbcLogout(void);
bool exec_sql(const std::string& stmt, bool critical);

#endif // MYODBC_H
