#ifndef ODBC_H
#define ODBC_H

#include <ostream>

#include "sqlext_compatible.h"

using namespace std;

extern SQLHSTMT serverhstmt;

bool odbc_login(ostream& out, const char* dataSource, const char* user, const char* passwd);
bool odbc_logout(ostream& info);

#endif // ODBC_H
