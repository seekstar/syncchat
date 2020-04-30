#include "dialogsignup.h"
#include "ui_dialogsignup.h"

DialogSignup::DialogSignup(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSignup)
{
    ui->setupUi(this);
}

DialogSignup::~DialogSignup()
{
    delete ui;
}
