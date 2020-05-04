#include "winlogin.h"
#include "ui_winlogin.h"

#include <QMessageBox>
#include <QDebug>

#include <openssl/sha.h>
#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "types.h"
#include "sslbase.h"

#include "myglobal.h"

WinLogin::WinLogin(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WinLogin),
    busy(false)
{
    ui->setupUi(this);

    connect(&dialogSignup, &DialogSignup::sigErr, this, &WinLogin::sigErr);
    connect(&dialogSignup, &DialogSignup::sigDone, &dialogSignup, &DialogSignup::close);
    //connect(ui->btn_signup, &QPushButton::clicked, dialogSignup, &DialogSignup::exec);
    connect(ui->btn_signup, &QPushButton::clicked, [&] {
        if (!busy) {
            dialogSignup.exec();
        }
    });
    connect(ui->btn_login, &QPushButton::clicked, this, &WinLogin::login);
}

void WinLogin::resetSock(ssl_socket *sock) {
    socket_ = sock;
    qDebug() << "socket_ in WinLogin resetted";
    dialogSignup.resetSock(sock);
}

void WinLogin::login() {
    if (busy) {
        qDebug() << "busy";
        return;
    }
    busy = true;

    struct C2SHeader *c2sHeader = reinterpret_cast<struct C2SHeader *>(buf_);
    struct LoginInfo *loginInfo = reinterpret_cast<struct LoginInfo *>(c2sHeader + 1);
    try {
        loginInfo->userid = boost::lexical_cast<userid_t>(ui->userid->text().toStdString());
    } catch (boost::bad_lexical_cast& e) {
        QMessageBox::information(this, "Error", QString("Illegal userid") + e.what());
        return;
    }
    SHA256(reinterpret_cast<const uint8_t*>(ui->pw->text().toStdString().c_str()), ui->pw->text().size(), loginInfo->pwsha256);
    c2sHeader->tsid = ++last_tsid;
    c2sHeader->type = C2S::LOGIN;
    qDebug() << "Before read_login_reply";
    boost::asio::async_write(*socket_,
        boost::asio::buffer(buf_, sizeof(C2SHeader) + sizeof(LoginInfo)),
        boost::bind(&WinLogin::read_login_reply, this,
                    boost::asio::placeholders::error));
}

#define HANDLE_ERROR    \
    if (error) {        \
        emit sigErr(std::string("Error in ") + __PRETTY_FUNCTION__ + ": " + error.message());\
        busy = false;   \
        return;         \
    }

void WinLogin::read_login_reply(const boost::system::error_code& error) {
    qDebug() << "read_login_reply";
    HANDLE_ERROR;
    boost::asio::async_read(*socket_,
        boost::asio::buffer(buf_, sizeof(S2CHeader)),
        boost::bind(&WinLogin::handle_login_reply, this, boost::asio::placeholders::error));
}
void WinLogin::handle_login_reply(const boost::system::error_code& error) {
    HANDLE_ERROR;
    struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(buf_);
    if (s2cHeader->tsid != last_tsid) {
        qDebug() << ("Warning: transaction id incorrect! Expected " + last_tsid);
    }
    switch (s2cHeader->type) {
    case S2C::LOGIN_OK:
        emit sigDone();
        break;
    case S2C::ALREADY_LOGINED:
        qDebug() << "Warning: ALREADY_LOGINED";
        break;
    case S2C::WRONG_PASSWORD:
        QMessageBox::information(this, "提示", "密码错误");
        break;
    default:
        qDebug() << (std::string("Warning in ") + __PRETTY_FUNCTION__ + ": Unrecognized type " +
                     std::to_string((S2CBaseType)s2cHeader->type)).c_str();
        break;
    }
    busy = false;
    qDebug() << "free now";
}

WinLogin::~WinLogin()
{
    delete ui;
}
