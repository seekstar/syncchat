#ifndef ODBC_H
#define ODBC_H

#include <ostream>

#include "sqlext_compatible.h"

extern SQLHSTMT serverhstmt;

bool odbc_login(std::ostream& out, const char* dataSource, const char* user, const char* passwd);
bool odbc_logout(std::ostream& info);

#endif // ODBC_H
