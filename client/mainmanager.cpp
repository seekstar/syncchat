#include "mainmanager.h"

#include "myodbc.h"

MainManager::MainManager(const char *ip, const char *port, QObject *parent)
    : QObject(parent),
      sslManager(ip, port, this)
{
    myodbcLogin();

    connect(&sslManager, &SslManager::sigErr, &dialogReconnect, &DialogReconnect::ShowErr);
    connect(&sslManager, &SslManager::sigDone, &dialogReconnect, &DialogReconnect::close);
    connect(&dialogReconnect, &DialogReconnect::sigRetry, &sslManager, &SslManager::sslconn);
    //connect(sslManager, &SslManager::sigDone, dialogSignup, &DialogSignup::show);
    connect(&sslManager, &SslManager::sigDone, &winLogin, &WinLogin::resetSock);
    connect(&sslManager, &SslManager::sigDone, &winLogin, &WinLogin::show);
    connect(&winLogin, &WinLogin::sigErr, &dialogReconnect, &DialogReconnect::ShowErr);

    sslManager.sslconn();
}

MainManager::~MainManager() {
    myodbcLogout();
}
