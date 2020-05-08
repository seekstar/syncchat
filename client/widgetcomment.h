#ifndef WIDGETCOMMENT_H
#define WIDGETCOMMENT_H

#include <QWidget>

#include "types.h"

namespace Ui {
class WidgetComment;
}

class WidgetComment : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetComment(QWidget *parent = 0);
    ~WidgetComment();

signals:
    void Reply(commentid_t);

public slots:
    void Set(CppComment coment);

private:
    Ui::WidgetComment *ui;

    commentid_t id;
};

#endif // WIDGETCOMMENT_H
