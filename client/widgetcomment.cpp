#include "widgetcomment.h"
#include "ui_widgetcomment.h"

WidgetComment::WidgetComment(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetComment)
{
    ui->setupUi(this);
}

WidgetComment::~WidgetComment()
{
    delete ui;
}
