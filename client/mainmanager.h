#ifndef MAINMANAGER_H
#define MAINMANAGER_H

#include <QObject>
#include <QDebug>

#include "dialogreconnect.h"
#include "sslmanager.h"

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

public slots:

private slots:
    void HandleAddFriendReply(userid_t userid, bool reply);

private:
    SslManager sslManager;
    DialogReconnect dialogReconnect;

    DialogSignup dialogSignup;
    WinLogin winLogin;
    MainWindow mainWindow;
    DialogAddFriend dialogAddFriend;
};

#endif // MAINMANAGER_H
