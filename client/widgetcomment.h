#ifndef WIDGETCOMMENT_H
#define WIDGETCOMMENT_H

#include <QWidget>

namespace Ui {
class WidgetComment;
}

class WidgetComment : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetComment(QWidget *parent = 0);
    ~WidgetComment();

private:
    Ui::WidgetComment *ui;
};

#endif // WIDGETCOMMENT_H
