#ifndef ODBC_H_
#define ODBC_H_

#include <ostream>

#include "sqlext_compatible.h"

extern SQLHSTMT serverhstmt;

bool odbc_login(std::ostream& out, const char* dataSource, const char* user, const char* passwd);
bool odbc_logout(std::ostream& info);

template <typename inctype>
inctype insert_auto_inc(const char *stmt) {
    SQLLEN length;
    SQLRETURN ret;
    inctype inc;
    SQLBindParameter(serverhstmt, 1, SQL_PARAM_OUTPUT, SQL_C_UBIGINT, SQL_BIGINT, 0, 0, &inc, 0, &length);
	ret = SQLExecDirect(serverhstmt, (SQLCHAR*)stmt, SQL_NTS);
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        return inc;
    } else {
        return 0;
	}
}

#endif // ODBC_H_
