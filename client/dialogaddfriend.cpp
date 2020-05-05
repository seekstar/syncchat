#include "dialogaddfriend.h"
#include "ui_dialogaddfriend.h"

#include <QMessageBox>

#include <boost/lexical_cast.hpp>

DialogAddFriend::DialogAddFriend(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogAddFriend)
{
    ui->setupUi(this);
    connect(ui->btn_AddFriend, &QPushButton::clicked, this, &DialogAddFriend::slotAddFriend);
}

void DialogAddFriend::slotAddFriend() {
    userid_t userid;
    try {
        userid = boost::lexical_cast<userid_t>(ui->userid->text().toStdString());
    } catch (boost::bad_lexical_cast& e) {
        QMessageBox::information(this, "Error", QString("Illegal userid") + e.what());
        return;
    }
    emit AddFriend(userid);
}

DialogAddFriend::~DialogAddFriend()
{
    delete ui;
}
