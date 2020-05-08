#include "widgetmoment.h"
#include "ui_widgetmoment.h"

WidgetMoment::WidgetMoment(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetMoment)
{
    ui->setupUi(this);
}

WidgetMoment::~WidgetMoment()
{
    delete ui;
}
