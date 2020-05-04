#ifndef DIALOGADDFRIEND_H
#define DIALOGADDFRIEND_H

#include <QDialog>

#include "types.h"

namespace Ui {
class DialogAddFriend;
}

class DialogAddFriend : public QDialog
{
    Q_OBJECT

public:
    explicit DialogAddFriend(QWidget *parent = 0);
    ~DialogAddFriend();

signals:
    void AddFriend(userid_t userid);

private:
    void slotAddFriend();

    Ui::DialogAddFriend *ui;
};

#endif // DIALOGADDFRIEND_H
