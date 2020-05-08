#include "dialogfindbyusername.h"
#include "ui_dialogfindbyusername.h"

DialogFindByUsername::DialogFindByUsername(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogFindByUsername)
{
    ui->setupUi(this);
    connect(ui->btn_find, &QPushButton::clicked, this, &DialogFindByUsername::slotFindByUsername);
}

DialogFindByUsername::~DialogFindByUsername()
{
    delete ui;
}

void DialogFindByUsername::slotFindByUsername() {
    emit FindByUsername(ui->username->text().toStdString());
}
