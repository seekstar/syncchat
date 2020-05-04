#include "dialogsignup.h"
#include "ui_dialogsignup.h"

#include <QMessageBox>
#include <QDebug>
#include <boost/bind.hpp>

#include "myglobal.h"
#include "myodbc.h"
#include "pushbuf.h"

DialogSignup::DialogSignup(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSignup)
{
    ui->setupUi(this);

    connect(ui->btn_signup, &QPushButton::clicked, this, &DialogSignup::slotSignup);
}

void DialogSignup::slotSignup() {
    std::vector<uint8_t> buf(sizeof(SignupHeader));
    struct SignupHeader *signupHeader = reinterpret_cast<struct SignupHeader *>(buf.data());

    std::string usernameIn = ui->username->text().toStdString();
    std::string phoneIn = ui->phone->text().toStdString();
    signupHeader->namelen = usernameIn.size();
    signupHeader->phonelen = phoneIn.size();
    signupHeader->signaturelen = 0;
    std::string pwIn = ui->pw->text().toStdString();
    SHA256(reinterpret_cast<const uint8_t*>(pwIn.c_str()), pwIn.size(), signupHeader->pwsha256);
    PushBuf(buf, usernameIn.c_str(), usernameIn.size());
    PushBuf(buf, phoneIn.c_str(), phoneIn.size());
    emit signup(buf);
}

void DialogSignup::signupRes(userid_t userid) {
    QMessageBox::information(this, "注册成功", QString("您的id为") + std::to_string(userid).c_str() + "\n请牢记，若遗失不可找回");
}

DialogSignup::~DialogSignup()
{
    delete ui;
}
