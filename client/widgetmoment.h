#ifndef WIDGETMOMENT_H
#define WIDGETMOMENT_H

#include <QWidget>

#include "dialogcomments.h"

#include "types.h"

namespace Ui {
class WidgetMoment;
}

class WidgetMoment : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetMoment(QWidget *parent = 0);
    ~WidgetMoment();

    void HandleComments(const std::vector<CppComment>& comments);

signals:
    void Comment(momentid_t to, commentid_t reply, CppContent content);
    void CommentsReq(momentid_t to);

public slots:
    void Set(CppMoment moment);

private slots:
    void slotComment(commentid_t reply, CppContent content);
    void slotCommentsReq();

private:
    Ui::WidgetMoment *ui;

    DialogComments dialogComments;
    momentid_t id;
};

#endif // WIDGETMOMENT_H
