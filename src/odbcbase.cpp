#include "odbcbase.h"
#include "cppbase.h"

#include <ostream>
#include <string.h>

SQLHENV henv = SQL_NULL_HENV;
SQLHDBC hdbc = SQL_NULL_HDBC;
SQLHSTMT hstmt = SQL_NULL_HSTMT;

bool odbc_init_context(std::ostream& err) {
    SQLRETURN retcode;
    //分配环境句柄
    retcode = SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&henv);
    if (!SQL_SUCCEEDED(retcode)) {
        err << "Alloc SQL_HANDLE_ENV error!\n";
        goto ERR_serverenv;
    }
    // Notify ODBC that this is an ODBC 3.0 app.
    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, SQL_IS_INTEGER);
    if(!SQL_SUCCEEDED(retcode))
    {
        err<<"AllocEnvHandle error!\n";
        goto ERR_serverenv;
    }
    //分配连接句柄
    retcode = SQLAllocHandle(SQL_HANDLE_DBC,henv,&hdbc);
    if(!SQL_SUCCEEDED(retcode))
    {
        err<<"AllocDbcHandle error!\n";
        goto ERR_HANDLE_DBC;
    }
    return 0;
ERR_HANDLE_DBC:
    //释放环境句柄句柄
    retcode=SQLFreeHandle(SQL_HANDLE_ENV,henv);
    if(!SQL_SUCCEEDED(retcode)) {
        err<<"free henv error!\n";
    }
ERR_serverenv:
    return 1;
}

bool odbc_free_context(std::ostream& err) {
    bool fail = false;
    //释放连接句柄
    SQLRETURN ret=SQLFreeHandle(SQL_HANDLE_DBC,hdbc);
    if(!SQL_SUCCEEDED(ret)) {
        err<<"free hdbc error!\n";
        fail = true;
    }
    //释放环境句柄句柄
    ret=SQLFreeHandle(SQL_HANDLE_ENV,henv);
    if(!SQL_SUCCEEDED(ret)) {
        err<<"free henv error!\n";
        fail = true;
    }
    return fail;
}

//dataSource can be ip or data source name
bool odbc_connect(std::ostream& err, const char* dataSource, const char* user, const char* passwd) {
    SQLRETURN ret;
    if (odbc_init_context(err)) {
        return 1;
    }
    //数据库连接
    ret = SQLConnect(hdbc,(SQLCHAR*)dataSource,SQL_NTS,(SQLCHAR*)user,SQL_NTS,(SQLCHAR*)passwd,SQL_NTS);//第二个参数是之前配置的数据源，后面是数据库用户名和密码
    if(!SQL_SUCCEEDED(ret))
    {
        err<<"SQL_Connect error!"<<std::endl;
        goto ERR_CONNECT;
    }
    //分配执行语句句柄
    ret = SQLAllocHandle(SQL_HANDLE_STMT,hdbc,&hstmt);
    if (!SQL_SUCCEEDED(ret)) {
        err << "Alloc SQL_HANDLE_STMT error!\n";
        goto ERR_HANDLE_STMT;
    }
    return 0;
ERR_HANDLE_STMT:
    //断开数据库连接
    ret=SQLDisconnect(hdbc);
    if(!SQL_SUCCEEDED(ret)) {
        err<<"disconnected error!\n";
    }
ERR_CONNECT:
    odbc_free_context(err);
    return 1;
}

bool odbc_driver_connect(std::ostream& err, const char *connStr) {
    if (odbc_init_context(err)) {
        return 1;
    }
    //数据库连接
    SQLCHAR outConnStr[255];
    SQLSMALLINT outConnStrLen;
    SQLRETURN retcode = SQLDriverConnect(hdbc, NULL, (SQLCHAR*)connStr, strlen(connStr), outConnStr,
                               sizeof(outConnStr), &outConnStrLen, SQL_DRIVER_COMPLETE);
    if (outConnStrLen) {
        dbgcout << outConnStr << '\n';
    }
    if (!SQL_SUCCEEDED(retcode)) {
        err << "Error: SQLDriverConnect\n";
        goto ERR_CONNECT;
    }
    //分配执行语句句柄
    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO)) {
        err << "Error: SQLAllocHandle\n";
        goto ERR_HANDLE_STMT;
    }
    return 0;
ERR_HANDLE_STMT:
    //断开数据库连接
    retcode=SQLDisconnect(hdbc);
    if(!SQL_SUCCEEDED(retcode)) {
        err<<"disconnected error!\n";
    }
ERR_CONNECT:
    odbc_free_context(err);
    return 1;
}

bool odbc_free_all(std::ostream& err) {
    bool fail = false;
    SQLRETURN ret;
    //释放语句句柄
    ret=SQLFreeHandle(SQL_HANDLE_STMT,hstmt);
    if(!SQL_SUCCEEDED(ret)) {
        err<<"free hstmt error!\n";
        fail = true;
    }
    //断开数据库连接
    ret=SQLDisconnect(hdbc);
    if(!SQL_SUCCEEDED(ret)) {
        err<<"disconnected error!\n";
        fail = true;
    }
    fail |= odbc_free_context(err);
    return fail;
}

void odbc_get_errmsg(std::ostream& err) {
    SQLINTEGER errnative;
    UCHAR errmsg[255];
    SQLSMALLINT errmsglen;
    UCHAR errstate[5];
    SQLGetDiagRec(SQL_HANDLE_STMT, hstmt,
            1, errstate,
            &errnative, errmsg, sizeof(errmsg), &errmsglen);
    err << "errstate: " << errstate << "\nerrnative: " << errnative << "\nerrmsg: " << errmsg << '\n';
}
bool odbc_exec(std::ostream& err, const char *stmt) {
    SQLRETURN ret = SQLExecDirect(hstmt, (SQLCHAR*)(stmt), SQL_NTS);
    //bool fail = (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO);
    bool fail = !SQL_SUCCEEDED(ret);
    if (fail) {
        err << "Error when executing: " << stmt << "\n";
        odbc_get_errmsg(err);
    } else {
        dbgcout << stmt << '\n';
    }
    return fail;
}

bool odbc_close_cursor(std::ostream& err) {
    SQLRETURN ret = SQLCloseCursor(hstmt);
    bool fail = !SQL_SUCCEEDED(ret);
    if (fail) {
        err << "Error in " << __PRETTY_FUNCTION__ << ": ";
        odbc_get_errmsg(err);
    } else {
        dbgcout << "odbc cursor closed\n";
    }
    return fail;
}
