#ifndef WIDGETMOMENT_H
#define WIDGETMOMENT_H

#include <QWidget>

namespace Ui {
class WidgetMoment;
}

class WidgetMoment : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetMoment(QWidget *parent = 0);
    ~WidgetMoment();

private:
    Ui::WidgetMoment *ui;
};

#endif // WIDGETMOMENT_H
