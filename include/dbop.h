#ifndef __DBOP_H__
#define __DBOP_H__

#include "types.h"
#include "odbc.h"

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

#endif  //__DBOP_H__