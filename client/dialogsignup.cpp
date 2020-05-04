#include "dialogsignup.h"
#include "ui_dialogsignup.h"

#include <QMessageBox>
#include <QDebug>
#include <boost/bind.hpp>

#include "myglobal.h"
#include "myodbc.h"

DialogSignup::DialogSignup(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSignup),
    busy(false)
{
    ui->setupUi(this);

    connect(ui->btn_signup, &QPushButton::clicked, this, &DialogSignup::signup);
}

void DialogSignup::resetSock(ssl_socket *sock) {
    socket_ = sock;
}

void DialogSignup::signup() {
    if (busy) {
        qDebug() << "DialogSignup is busy";
        return;
    }
    busy = true;

    struct C2SHeader *c2sHeader = reinterpret_cast<struct C2SHeader *>(buf_);
    struct SignupHeader *signupHeader = reinterpret_cast<struct SignupHeader *>(c2sHeader + 1);
    char *username = reinterpret_cast<char*>(signupHeader + 1);
    c2sHeader->tsid = ++last_tsid;
    c2sHeader->type = C2S::SIGNUP;

    std::string usernameIn = ui->username->text().toStdString();
    std::string phoneIn = ui->phone->text().toStdString();
    signupHeader->namelen = usernameIn.size();
    signupHeader->phonelen = phoneIn.size();
    signupHeader->signaturelen = 0;
    std::string pwIn = ui->pw->text().toStdString();
    SHA256(reinterpret_cast<const uint8_t*>(pwIn.c_str()), pwIn.size(), signupHeader->pwsha256);
    memcpy(username, usernameIn.c_str(), usernameIn.size());
    memcpy(username + usernameIn.size(), phoneIn.c_str(), phoneIn.size());
    boost::asio::async_write(*socket_,
        boost::asio::buffer(buf_, sizeof(C2SHeader) + sizeof(SignupHeader) + usernameIn.size() + phoneIn.size()),
        boost::bind(&DialogSignup::read_signup_reply, this, boost::asio::placeholders::error));
}

#define HANDLE_ERROR    \
    if (error) {        \
        emit sigErr(std::string("Error in ") + __PRETTY_FUNCTION__ + ": " + error.message());\
        busy = false;   \
        return;         \
    }
void DialogSignup::read_signup_reply(const boost::system::error_code& error) {
    HANDLE_ERROR;
    boost::asio::async_read(*socket_,
        boost::asio::buffer(buf_, sizeof(S2CHeader)),
        boost::bind(&DialogSignup::handle_signup_reply, this, boost::asio::placeholders::error));
}

void DialogSignup::handle_signup_reply(const boost::system::error_code& error) {
    HANDLE_ERROR;
    struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(buf_);
    if (s2cHeader->tsid != last_tsid) {
        qDebug() << "Warning: transaction id incorrect! Expected " << last_tsid;
    }
    switch (s2cHeader->type) {
    case S2C::USERNAME_TOO_LONG:
        QMessageBox::information(this, "提示", "用户名过长");
        break;
    case S2C::PHONE_TOO_LONG:
        QMessageBox::information(this, "提示", "手机号码过长");
        break;
    case S2C::SIGNUP_RESP:
        boost::asio::async_read(*socket_,
            boost::asio::buffer(buf_, sizeof(SignupReply)),
            boost::bind(&DialogSignup::handle_signup_reply2, this, boost::asio::placeholders::error));
    default:
        qDebug() << "Warning in " << __PRETTY_FUNCTION__ << ": Unrecognized type " <<
                    //std::to_string(reinterpret_cast<S2CBaseType>(s2cHeader->type)).c_str();
                    (S2CBaseType)s2cHeader->type;
        break;
    }
}

void DialogSignup::handle_signup_reply2(const boost::system::error_code& error) {
    HANDLE_ERROR;
    struct SignupReply *signupReply = reinterpret_cast<struct SignupReply *>(buf_);
    QMessageBox::information(this, "注册成功", QString("您的id为") + std::to_string(signupReply->id).c_str() + "\n请牢记，若遗失不可找回");
}

DialogSignup::~DialogSignup()
{
    delete ui;
}
