#include "dialogcreategroup.h"
#include "ui_dialogcreategroup.h"

DialogCreateGroup::DialogCreateGroup(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogCreateGroup)
{
    ui->setupUi(this);
    connect(ui->btn_creategroup, &QPushButton::clicked, this, &DialogCreateGroup::slotCreateGroup);
}

void DialogCreateGroup::slotCreateGroup() {
    emit CreateGroup(ui->groupname->text().toStdString());
}

DialogCreateGroup::~DialogCreateGroup()
{
    delete ui;
}
