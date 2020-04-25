#include "odbc.h"

SQLHENV serverhenv;
SQLHDBC serverhdbc;
SQLHSTMT serverhstmt;

//dataSource can be ip or data source name
bool odbc_login(std::ostream& errInfo, const char* dataSource, const char* user, const char* passwd) {
    SQLRETURN ret;

    //分配环境句柄
    ret = SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&serverhenv);
    if (!SQL_SUCCEEDED(ret)) {
        errInfo << "Alloc SQL_HANDLE_ENV error!\n";
        goto ERR_serverenv;
    }

    //设置环境属性
    ret = SQLSetEnvAttr(serverhenv,SQL_ATTR_ODBC_VERSION,(void*)SQL_OV_ODBC3,0);
    if(!SQL_SUCCEEDED(ret))
    {
        errInfo<<"AllocEnvHandle error!"<<std::endl;
        goto ERR_serverenv;
    }

    //分配连接句柄
    ret = SQLAllocHandle(SQL_HANDLE_DBC,serverhenv,&serverhdbc);
    if(!SQL_SUCCEEDED(ret))
    {
        errInfo<<"AllocDbcHandle error!"<<std::endl;
        goto ERR_HANDLE_DBC;
    }

    //数据库连接
    ret = SQLConnect(serverhdbc,(SQLCHAR*)dataSource,SQL_NTS,(SQLCHAR*)user,SQL_NTS,(SQLCHAR*)passwd,SQL_NTS);//第二个参数是之前配置的数据源，后面是数据库用户名和密码
    if(!SQL_SUCCEEDED(ret))
    {
        errInfo<<"SQL_Connect error!"<<std::endl;
        goto ERR_CONNECT;
    }

    //分配执行语句句柄
    ret = SQLAllocHandle(SQL_HANDLE_STMT,serverhdbc,&serverhstmt);
    if (!SQL_SUCCEEDED(ret)) {
        errInfo << "Alloc SQL_HANDLE_STMT error!\n";
        goto ERR_HANDLE_STMT;
    }
    return 0;

ERR_HANDLE_STMT:
    //断开数据库连接
    ret=SQLDisconnect(serverhdbc);
    if(SQL_SUCCESS!=ret&&SQL_SUCCESS_WITH_INFO!=ret) {
        errInfo<<"disconnected error!"<<std::endl;
        return 1;
    }
ERR_CONNECT:
    //释放连接句柄
    ret=SQLFreeHandle(SQL_HANDLE_DBC,serverhdbc);
    if(SQL_SUCCESS!=ret&&SQL_SUCCESS_WITH_INFO!=ret) {
        errInfo<<"free hdbc error!"<<std::endl;
        return 1;
    }
ERR_HANDLE_DBC:
    //释放环境句柄句柄
    ret=SQLFreeHandle(SQL_HANDLE_ENV,serverhenv);
    if(SQL_SUCCESS!=ret&&SQL_SUCCESS_WITH_INFO!=ret) {
        errInfo<<"free henv error!"<<std::endl;
        return 1;
    }
ERR_serverenv:
    return 1;
}

bool odbc_logout(std::ostream& errInfo) {
    SQLRETURN ret;

    //释放语句句柄
    ret=SQLFreeHandle(SQL_HANDLE_STMT,serverhstmt);
    if(SQL_SUCCESS!=ret && SQL_SUCCESS_WITH_INFO != ret) {
        errInfo<<"free hstmt error!"<<std::endl;
        return 1;
    }
    //断开数据库连接
    ret=SQLDisconnect(serverhdbc);
    if(SQL_SUCCESS!=ret&&SQL_SUCCESS_WITH_INFO!=ret) {
        errInfo<<"disconnected error!"<<std::endl;
        return 1;
    }
    //释放连接句柄
    ret=SQLFreeHandle(SQL_HANDLE_DBC,serverhdbc);
    if(SQL_SUCCESS!=ret&&SQL_SUCCESS_WITH_INFO!=ret) {
        errInfo<<"free hdbc error!"<<std::endl;
        return 1;
    }
    //释放环境句柄句柄
    ret=SQLFreeHandle(SQL_HANDLE_ENV,serverhenv);
    if(SQL_SUCCESS!=ret&&SQL_SUCCESS_WITH_INFO!=ret) {
        errInfo<<"free henv error!"<<std::endl;
        return 1;
    }
    return 0;
}
