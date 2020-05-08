#ifndef DIALOGCOMMENTS_H
#define DIALOGCOMMENTS_H

#include <QDialog>

namespace Ui {
class DialogComments;
}

class DialogComments : public QDialog
{
    Q_OBJECT

public:
    explicit DialogComments(QWidget *parent = 0);
    ~DialogComments();

private:
    Ui::DialogComments *ui;
};

#endif // DIALOGCOMMENTS_H
