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

void WinLogin::login() {
    if (busy) {
        qDebug() << "busy";
        return;
    }

    struct C2SHeader *c2sHeader = reinterpret_cast<struct C2SHeader *>(buf_);
    struct LoginInfo *loginInfo = reinterpret_cast<struct LoginInfo *>(c2sHeader + 1);
    try {
        loginInfo->userid = boost::lexical_cast<userid_t>(ui->userid->text().toStdString());
    } catch (boost::bad_lexical_cast& e) {
        QMessageBox::information(this, "Error", QString("Illegal userid") + e.what());
        return;
    }
    SHA256(ui->pw->text().toStdString().c_str(), ui->pw->text().size(), logininfo->pwsha256);
    c2sHeader->tsid = ++last_tsid;
    c2sHeader->type = C2S::LOGIN;
    boost::asio::async_write(*socket_,
        boost::asio::buffer(buf_, sizeof(C2SHeader) + sizeof(LoginInfo)),
        boost::bind(&WinLogin::read_login_reply, this,
                    boost::asio::placeholders::error));
}
void WinLogin::handle_error(const boost::system::error_code& error, const char *where) {
    qDebug() << ("Error in " + where + ": " + error.message());

    reconnect();
}

#define HANDLE_ERROR    \
    if (error) {        \
        handle_error(error, __PRETTY_FUNCTION__);\
        return;         \
    }

//TODO: Make full use of transaction id to clear the busy status sooner
void WinLogin::read_login_reply(const boost::system::error_code& error) {
    HANDLE_ERROR;
    boost::asio::async_read(*socket_,
        boost::asio::buffer(buf_, sizeof(S2CHeader)),
        boost::bind(&WinLogin::handle_login_reply, this, boost::asio::placeholders::error));
}
void WinLogin::handle_login_reply(const boost::system::error_code& error) {
    HANDLE_ERROR;
    struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(buf_);
    if (s2cHeader->tsid != last_tsid) {
        qDebug() << ("Warning: transaction id uncorrect! Expected " + last_tsid);
    }
    switch (s2cHeader->type) {
    case S2C::LOGIN_OK:
        mainWindow = new MainWindow;
        const char *dataSource = "wechatclient";
        std::string errorInfo = mainWindow->login(dataSource);
        if (errorInfo != "") {
            QMessageBox::critical(this, "Login error", QString(errorInfo.c_str()) +
                                  "\nPlease make sure that there is an odbc data source named " + dataSource + '!');
            mainWindow->deleteLater();
        }
        break;
    }
}

WinLogin::~WinLogin()
{
    delete ui;
}
