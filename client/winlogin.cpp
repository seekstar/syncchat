#include "winlogin.h"
#include "ui_winlogin.h"

#include <QMessageBox>

#include "dialogsignup.h"

#include <openssl/sha.h>
#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "types.h"
#include "sslclient.h"

WinLogin::WinLogin(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WinLogin),
    busy(false)
{
    ui->setupUi(this);

    connect(ui->btn_signup, &QPushButton::clicked, [this]{
        DialogSignup signup(this);
        signup.exec();
    });
    connect(ui->btn_login, &QPushButton::clicked, this, &WinLogin::login);
}

void WinLogin::read_login_reply(const boost::system::error_code& error) {
    if (error) {
        //TODO
    }
    //boost::asio::
}

void WinLogin::login() {
    if (busy)
        return;
    busy = true;

    const size_t sendsz = sizeof(C2S) + sizeof(struct logininfo);
    static_assert(sendsz <= BUFSIZE, "BUFSIZE is not enough for login!");
    try {
        struct logininfo *logininfo = reinterpret_cast<struct logininfo *>(buf_ + sizeof(C2S));
        logininfo->userid = boost::lexical_cast<userid_t>(ui->userid->text().toStdString());
        SHA256(ui->pw->text().toStdString().c_str(), ui->pw->text().size(), logininfo->pwsha256);
        *reinterpret_cast<C2S*>(buf_) = C2S::LOGIN;
        boost::asio::async_write(*socket_,
            boost::asio::buffer(buf_, sendsz),
            boost::bind(&WinLogin::read_login_reply, this,
                        boost::asio::placeholders::error));
    } catch (boost::bad_lexical_cast& e) {
        QMessageBox::information(this, "Error", e.what());
    }
}

WinLogin::~WinLogin()
{
    delete ui;
}
