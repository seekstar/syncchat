#ifndef DIALOGJOINGROUP_H
#define DIALOGJOINGROUP_H

#include <QDialog>

#include "types.h"

namespace Ui {
class DialogJoinGroup;
}

class DialogJoinGroup : public QDialog
{
    Q_OBJECT

public:
    explicit DialogJoinGroup(QWidget *parent = 0);
    ~DialogJoinGroup();

signals:
    void JoinGroup(grpid_t);

private slots:
    void slotJoinGroup();

private:
    Ui::DialogJoinGroup *ui;
};

#endif // DIALOGJOINGROUP_H
