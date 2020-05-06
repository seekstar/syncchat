#ifndef SSLIO_H_
#define SSLIO_H_

#include "sslbase.h"
#include <vector>

class SslIO {
public:
    SslIO(io_service_t& io_service, context_t& context);
    virtual ~SslIO();

    //ssl_socket::lowest_layer_type& socket();
    void SendLater(const void *data, size_t len);
    virtual void handle_sslio_error(const boost::system::error_code& error);

protected:
    ssl_socket socket_;

private:
    void StartSend();
    void handle_send(const boost::system::error_code& error);

    bool busy;
    std::vector<uint8_t> sending, sendbuf;
};

#endif	//SSLIO_H_
