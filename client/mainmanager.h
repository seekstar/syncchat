#ifndef MAINMANAGER_H
#define MAINMANAGER_H

#include <QObject>

#include "dialogreconnect.h"
#include "sslmanager.h"

#include "winlogin.h"
#include "mainwindow.h"

class MainManager : public QObject
{
    Q_OBJECT
public:
    explicit MainManager(const char *ip, const char *port, QObject *parent = 0);
    ~MainManager();

signals:

public slots:

private:
    SslManager sslManager;
    DialogReconnect dialogReconnect;

    //DialogSignup dialogSignup;
    WinLogin winLogin;
    MainWindow mainWindow;
};

#endif // MAINMANAGER_H
