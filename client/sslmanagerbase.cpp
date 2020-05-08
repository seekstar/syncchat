#include "sslmanager.h"
#include "sslmanagerbase.h"

#include <QDebug>

#include <iostream>
#include <boost/bind.hpp>

#include "pushbuf.h"
#include "cppbase.h"

SslManager::SslManager(const char *ip, const char *port)
    : //QThread(parent),
      //work(new boost::asio::io_service::work(io_service)),
      resolver(io_service),
      query(ip, port),
      iterator(resolver.resolve(query)),
      context(NULL),
      //socket_(io_service, context),
      socket_(NULL),
      busy(false),
      last_tsid(0)
{
    moveToThread(this);
    //setParent(parent);
    connect(&timer, &QTimer::timeout, [&] {
        //qDebug() << "poll";
        io_service.poll();
    });
    timer.start(100);
}

SslManager::~SslManager() {
}
//void SslManager::stop_io_service() {
//    work.reset();
//}

void SslManager::run() {
    qDebug() << "The thread id of sslManager is " << QThread::currentThreadId();
    sslconn();
//    io_service.run(); //won't return because it's listening
//    io_service.reset();
//    qDebug() << "io_service.run() returned";
    exec();
}

//void SslManager::run_io_service() {
//    qDebug() << __PRETTY_FUNCTION__;

//    if (io_service.stopped()) {
//        qDebug() << "io_service.run()";
//        io_service.run();
//        io_service.reset();
//        qDebug() << "io_service.run() returned";
//    } else {
//        qDebug() << "io_service is already running";
//    }
//}

//TODO: Support call back function to avoid copy when not busy
void SslManager::SendLater(const void *data, size_t len) {
    if (busy) {
        size_t oldsz = sendbuf.size();
        sendbuf.resize(oldsz + len);
        memcpy(sendbuf.data() + oldsz, data, len);
    } else {
        sending.resize(len);
        memcpy(sending.data(), data, len);
        busy = true;
        StartSend();
    }
}
void SslManager::StartSend() {
    qDebug() << "sending " << sending.size() << " bytes";
    boost::asio::async_write(*socket_,
        boost::asio::buffer(sending),
        boost::bind(&SslManager::handle_send, this, boost::asio::placeholders::error));
}
void SslManager::handle_send(const boost::system::error_code& error) {
    HANDLE_ERROR;
    sending.clear();
    if (sendbuf.empty()) {
        busy = false;
    } else {
        swap(sendbuf, sending);
        StartSend();
    }
}
std::vector<uint8_t> SslManager::C2SHeaderBuf(C2S type) {
    transactionType_[last_tsid+1] = type;
    return C2SHeaderBuf_noreply(type);
}
std::vector<uint8_t> SslManager::C2SHeaderBuf_noreply(C2S type) {
    std::vector<uint8_t> buf(sizeof(C2SHeader));
    struct C2SHeader *c2sHeader = reinterpret_cast<struct C2SHeader *>(buf.data());
    c2sHeader->tsid = ++last_tsid;
    c2sHeader->type = type;
    return buf;
}

void SslManager::SendLater(const std::vector<uint8_t>& buf) {
    SendLater(buf.data(), buf.size());
}

void SslManager::handle_handshake(const boost::system::error_code& error)
{
    HANDLE_ERROR;
    qDebug() << "handshake done";
    emit sigDone();
    busy = false;
    ListenToServer();
}

void SslManager::handle_connect(const boost::system::error_code& error)
{
    //std::cerr << "After async call, the thread id is " << std::this_thread::get_id() << std::endl;
    qDebug() << "Enter handle_connect";
    HANDLE_ERROR;
    socket_->async_handshake(boost::asio::ssl::stream_base::client,
                             boost::bind(&SslManager::handle_handshake, this,
                                         boost::asio::placeholders::error));
}

bool SslManager::verify_certificate(bool preverified,
                        boost::asio::ssl::verify_context& ctx)
{
    // The verify callback can be used to check whether the certificate that is
    // being presented is valid for the peer. For example, RFC 2818 describes
    // the steps involved in doing this for HTTPS. Consult the OpenSSL
    // documentation for more details. Note that the callback is called once
    // for each certificate in the certificate chain, starting from the root
    // certificate authority.

    // In this example we will simply print the certificate's subject name.
    char subject_name[256];
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    qDebug() << "Verifying" << subject_name;

    return preverified;
}

void SslManager::sslconn() {
    qDebug() << "sslconn";
    if (busy) {
        qDebug() << "SslManager busy";
        return;
    }
    busy = true;

    //std::cerr << "The thread id now is " << std::this_thread::get_id() << std::endl;

    if (context) {
        delete context;
    }
    context = new context_t(boost::asio::ssl::context::sslv23);
    system("pwd");
    context->load_verify_file("ca.pem");
    if (socket_) {
        delete socket_;
    }
    socket_ = new ssl_socket(io_service, *context);
    socket_->set_verify_mode(boost::asio::ssl::verify_peer);
    socket_->set_verify_callback(boost::bind(&SslManager::verify_certificate, this, _1, _2));
    qDebug() << "before async_connect";
    boost::asio::async_connect(socket_->lowest_layer(), iterator,
                               boost::bind(&SslManager::handle_connect, this, boost::asio::placeholders::error));
    //run_io_service();
}

void SslManager::HandleInfoHeader(const boost::system::error_code& error) {
    HANDLE_ERROR;
    uint64_t len = *reinterpret_cast<uint64_t *>(recvbuf_);
    boost::asio::async_read(*socket_,
        boost::asio::buffer(recvbuf_ + sizeof(uint64_t), len),
        boost::bind(&SslManager::HandleInfoContent, this, boost::asio::placeholders::error));
}
void SslManager::HandleInfoContent(const boost::system::error_code &error) {
    HANDLE_ERROR;
    //TODO: Handle recvbuf not long enough
    uint64_t len = *reinterpret_cast<uint64_t *>(recvbuf_);
    char *content = reinterpret_cast<char *>(recvbuf_ + sizeof(uint64_t));
    emit info(std::string(content, len));
    ListenToServer();
}
