#include "dialogmoments.h"
#include "ui_dialogmoments.h"

DialogMoments::DialogMoments(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogMoments)
{
    ui->setupUi(this);
    connect(ui->btn_edit, &QPushButton::clicked, this, &DialogMoments::sigEditMoment);
}

DialogMoments::~DialogMoments()
{
    delete ui;
}
