#ifndef DIALOGMOMENTS_H
#define DIALOGMOMENTS_H

#include <QDialog>
//#include <QListWidgetItem>
#include "widgetmoment.h"

#include <unordered_map>

#include "types.h"

namespace Ui {
class DialogMoments;
}

class DialogMoments : public QDialog
{
    Q_OBJECT

public:
    explicit DialogMoments(QWidget *parent = 0);
    ~DialogMoments();

signals:
    void sigEditMoment();
    void MomentsReq();
    void Comment(momentid_t to, commentid_t reply, CppContent content);
    void CommentsReq(momentid_t to);

public slots:
    void HandleShow();
    void HandleMoment(CppMoment moment);
    void HandleMoments(std::vector<CppMoment>);
    void HandleComments(momentid_t momentid, std::vector<CppComment> comments);
private:
    Ui::DialogMoments *ui;
    //QTimer timer;

    std::unordered_map<momentid_t, WidgetMoment*> mmtWidget_;
};

#endif // DIALOGMOMENTS_H
