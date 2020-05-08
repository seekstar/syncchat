#ifndef DIALOGCOMMENTS_H
#define DIALOGCOMMENTS_H

#include <QDialog>
#include <QTimer>

#include "types.h"

namespace Ui {
class DialogComments;
}

class DialogComments : public QDialog
{
    Q_OBJECT

public:
    explicit DialogComments(QWidget *parent = 0);
    ~DialogComments();

    void HandleComments(std::vector<CppComment> comments);
    void HandleComment(const CppComment& comment);

signals:
    void Comment(commentid_t reply, CppContent content);
    void CommentsReq();

public slots:
    void HandleShow();

private slots:
    void slotSend();
    void HandleReply(commentid_t id);

private:
    Ui::DialogComments *ui;
    //QTimer timer;

    commentid_t reply;
};

#endif // DIALOGCOMMENTS_H
