#ifndef ODBC_H_
#define ODBC_H_

#include <ostream>

#include "sqlext_compatible.h"

extern SQLHSTMT hstmt;

bool odbc_connect(std::ostream& err, const char* dataSource, const char* user, const char* passwd);
bool odbc_driver_connect(std::ostream& err, const char *connStr);
bool odbc_free_all(std::ostream& err);
bool odbc_exec(std::ostream& err, const char *stmt);
bool odbc_close_cursor(std::ostream& err);

#endif // ODBC_H_
