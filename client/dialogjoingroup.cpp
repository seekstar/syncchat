#include "dialogjoingroup.h"
#include "ui_dialogjoingroup.h"

#include <QMessageBox>

#include <boost/lexical_cast.hpp>

DialogJoinGroup::DialogJoinGroup(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogJoinGroup)
{
    ui->setupUi(this);
    connect(ui->btn_JoinGroup, &QPushButton::clicked, this, &DialogJoinGroup::slotJoinGroup);
}

DialogJoinGroup::~DialogJoinGroup()
{
    delete ui;
}

void DialogJoinGroup::slotJoinGroup() {
    try {
        grpid_t grpid = boost::lexical_cast<grpid_t>(ui->grpid->text().toStdString());
        emit JoinGroup(grpid);
    } catch (boost::bad_lexical_cast& e) {
        QMessageBox::information(this, "Error", QString("Illegal group id\n") + e.what());
    }
}
