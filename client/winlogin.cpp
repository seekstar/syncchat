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
    ui(new Ui::WinLogin)
{
    ui->setupUi(this);

    connect(ui->btn_signup, &QPushButton::clicked, this, &WinLogin::signup);
    connect(ui->btn_login, &QPushButton::clicked, this, &WinLogin::slotLogin);
}

void WinLogin::slotLogin() {
    struct LoginInfo loginInfo;
    try {
        loginInfo.userid = boost::lexical_cast<userid_t>(ui->userid->text().toStdString());
    } catch (boost::bad_lexical_cast& e) {
        QMessageBox::information(this, "Error", QString("Illegal userid") + e.what());
        return;
    }
    std::string pw = ui->pw->text().toStdString();
    SHA256(reinterpret_cast<const uint8_t*>(pw.c_str()), pw.size(), loginInfo.pwsha256);
    emit login(loginInfo);
    qDebug() << "signal \"login\" emitted";
}

WinLogin::~WinLogin()
{
    delete ui;
}
