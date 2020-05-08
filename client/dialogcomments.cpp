#include "dialogcomments.h"
#include "ui_dialogcomments.h"

DialogComments::DialogComments(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogComments)
{
    ui->setupUi(this);
}

DialogComments::~DialogComments()
{
    delete ui;
}
