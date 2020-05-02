#include "mainmanager.h"

MainManager::MainManager(QObject *parent) : QObject(parent)
{
    connect(timer, SIGNAL(timeout()), io_)
}
