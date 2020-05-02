#ifndef SSLBASE_H
#define SSLBASE_H

#include "sslbase.h"

extern ssl_socket *socket_;
extern transaction_t last_tsid = 0;

void sslconn();
void reconnect();

#endif // SSLBASE_H
