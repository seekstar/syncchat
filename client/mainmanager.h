#ifndef MAINMANAGER_H
#define MAINMANAGER_H

#include <QObject>
#include <QDebug>

#include "dialogreconnect.h"
#include "sslmanager.h"

#include <unordered_map>

#include "winlogin.h"
#include "mainwindow.h"
#include "dialogaddfriend.h"

class MainManager : public QObject
{
    Q_OBJECT
public:
    explicit MainManager(const char *ip, const char *port, QObject *parent = 0);
    ~MainManager();

signals:
    void UserPublicInfoReq(userid_t);

public slots:

private slots:
    void HandleAddFriendReply(userid_t userid, bool reply);
    void HandleUserPublicInfoReply(userid_t userid, std::string username);

private:
    SslManager sslManager;
    DialogReconnect dialogReconnect;

    DialogSignup dialogSignup;
    WinLogin winLogin;
    MainWindow mainWindow;
    DialogAddFriend dialogAddFriend;

    std::unordered_map<userid_t, std::string> usernames;
};

#endif // MAINMANAGER_H
