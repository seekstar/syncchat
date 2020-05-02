#ifndef MAINMANAGER_H
#define MAINMANAGER_H

#include <QObject>

#include "mainwindow.h"
#include "dialogreconnect.h"
#include "sslmanager.h"

class MainManager : public QObject
{
    Q_OBJECT
public:
    explicit MainManager(QObject *parent = 0);

signals:

public slots:

private:
    DialogReconnect dialogReconnect;
    SslManager sslManager;
    MainWindow mainWindow;
    QTimer qtimer;
};

#endif // MAINMANAGER_H
