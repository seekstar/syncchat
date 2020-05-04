#include "dialogreconnect.h"
#include "ui_dialogreconnect.h"

DialogReconnect::DialogReconnect(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogReconnect)
{
    ui->setupUi(this);
    connect(ui->btn_retry, &QPushButton::clicked, this, &DialogReconnect::sigRetry);
    connect(ui->btn_retry, &QPushButton::clicked, this, &DialogReconnect::close);
}

void DialogReconnect::ShowErr(std::string msg) {
    ui->label->setText(QString("服务器似乎开小差了") + '\n' + msg.c_str());
    show();
}

//void DialogReconnect::retry() {
//    //QUESTION: Might be trapped in io_service.run?
//    reconnect();
//}

DialogReconnect::~DialogReconnect()
{
    delete ui;
}
