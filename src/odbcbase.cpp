#include "odbcbase.h"
#include "cppbase.h"

#include <ostream>

SQLHENV serverhenv;
SQLHDBC serverhdbc;
SQLHSTMT serverhstmt;

bool odbc_init_context(std::ostream& err) {
    SQLRETURN ret;
    //分配环境句柄
    ret = SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&serverhenv);
    if (!SQL_SUCCEEDED(ret)) {
        err << "Alloc SQL_HANDLE_ENV error!\n";
        goto ERR_serverenv;
    }
    //设置环境属性
    ret = SQLSetEnvAttr(serverhenv,SQL_ATTR_ODBC_VERSION,(void*)SQL_OV_ODBC3,0);
    if(!SQL_SUCCEEDED(ret))
    {
        err<<"AllocEnvHandle error!\n";
        goto ERR_serverenv;
    }
    //分配连接句柄
    ret = SQLAllocHandle(SQL_HANDLE_DBC,serverhenv,&serverhdbc);
    if(!SQL_SUCCEEDED(ret))
    {
        err<<"AllocDbcHandle error!\n";
        goto ERR_HANDLE_DBC;
    }
    return 0;
ERR_HANDLE_DBC:
    //释放环境句柄句柄
    ret=SQLFreeHandle(SQL_HANDLE_ENV,serverhenv);
    if(!SQL_SUCCEEDED(ret)) {
        err<<"free henv error!\n";
    }
ERR_serverenv:
    return 1;
}

bool odbc_free_context(std::ostream& err) {
    bool fail = false;
    //释放连接句柄
    ret=SQLFreeHandle(SQL_HANDLE_DBC,serverhdbc);
    if(!SQL_SUCCEEDED(ret)) {
        err<<"free hdbc error!\n";
        fail = true;
    }
    //释放环境句柄句柄
    ret=SQLFreeHandle(SQL_HANDLE_ENV,serverhenv);
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
    ret = SQLConnect(serverhdbc,(SQLCHAR*)dataSource,SQL_NTS,(SQLCHAR*)user,SQL_NTS,(SQLCHAR*)passwd,SQL_NTS);//第二个参数是之前配置的数据源，后面是数据库用户名和密码
    if(!SQL_SUCCEEDED(ret))
    {
        err<<"SQL_Connect error!"<<std::endl;
        goto ERR_CONNECT;
    }
    //分配执行语句句柄
    ret = SQLAllocHandle(SQL_HANDLE_STMT,serverhdbc,&serverhstmt);
    if (!SQL_SUCCEEDED(ret)) {
        err << "Alloc SQL_HANDLE_STMT error!\n";
        goto ERR_HANDLE_STMT;
    }
    return 0;
ERR_HANDLE_STMT:
    //断开数据库连接
    ret=SQLDisconnect(serverhdbc);
    if(!SQL_SUCCEEDED(ret)) {
        err<<"disconnected error!\n";
    }
ERR_CONNECT:
    odbc_free_context(err);
    return 1;
}

bool odbc_driver_connect(std::ostream& err, const char *connStr) {
    //数据库连接
    SQLCHAR outConnStr[255];
    SQLSMALLINT outConnStrLen;
    retcode = SQLDriverConnect(hdbc, NULL, (SQLCHAR*)connStr, sizeof(connStr), outConnStr,
                               sizeof(outConnStr), &outConnStrLen, SQL_DRIVER_COMPLETE);
    if (outConnStrLen) {
        dbgcout << outConnStr << std::endl;
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
    ret=SQLDisconnect(serverhdbc);
    if(!SQL_SUCCEEDED(ret)) {
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
    ret=SQLFreeHandle(SQL_HANDLE_STMT,serverhstmt);
    if(!SQL_SUCCEEDED(ret)) {
        err<<"free hstmt error!\n";
        fail = true;
    }
    //断开数据库连接
    ret=SQLDisconnect(serverhdbc);
    if(!SQL_SUCCEEDED(ret)) {
        err<<"disconnected error!\n";
        fail = true;
    }
    fail |= odbc_free_context(err);
    return fail;
}

bool odbc_exec(std::ostream& err, const char *stmt) {
    SQLRETURN ret = SQLExecDirect(serverhstmt, (SQLCHAR*)(stmt), SQL_NTS);
    bool fail = (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO);
    if (fail) {
        SQLINTEGER errnative;
        UCHAR errmsg[255];
        SQLSMALLINT errmsglen;
        UCHAR errstate[5];
        SQLGetDiagRec(SQL_HANDLE_STMT, serverhstmt,
               1, errstate,
               &errnative, errmsg, sizeof(errmsg), &errmsglen);
        err << "Error when executing: " << stmt << "\nerrstate: " << errstate << "\nerrnative: " << errnative << "\nerrmsg: " << errmsg;
    }
    return fail;
}
