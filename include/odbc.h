#ifndef ODBC_H_
#define ODBC_H_

#include <ostream>

#include "sqlext_compatible.h"

extern SQLHSTMT serverhstmt;

bool odbc_login(std::ostream& out, const char* dataSource, const char* user, const char* passwd);
bool odbc_logout(std::ostream& info);
bool odbc_exec(std::ostream& err, const char *stmt);

#endif // ODBC_H_
