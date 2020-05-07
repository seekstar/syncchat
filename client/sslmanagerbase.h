#ifndef SSLMANAGERBASE_H
#define SSLMANAGERBASE_H

#include "sslmanager.h"

#include <QDebug>
#include <boost/bind.hpp>
#include <pushbuf.h>

#define HANDLE_ERROR        \
    if (error) {            \
        emit sigErr(std::string("Error: ") + __PRETTY_FUNCTION__ + "\n" + error.message());\
        busy = false;       \
        delete context;     \
        context = NULL;     \
        delete socket_;     \
        socket_ = NULL;     \
        sending.clear();    \
        sendbuf.clear();    \
        return;             \
    }

#endif // SSLMANAGERBASE_H
