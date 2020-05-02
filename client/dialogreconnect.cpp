#include "dialogreconnect.h"
#include "ui_dialogreconnect.h"

#include "sslclient.h"

DialogReconnect::DialogReconnect(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogReconnect)
{
    ui->setupUi(this);

    connect(ui->btn_retry, &DialogReconnect::retry, )
}

void DialogReconnect::retry() {
    //QUESTION: Might be trapped in io_service.run?
    reconnect();
}

DialogReconnect::~DialogReconnect()
{
    delete ui;
}
