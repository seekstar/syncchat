#ifndef DIALOGMOMENTS_H
#define DIALOGMOMENTS_H

#include <QDialog>

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

private:
    Ui::DialogMoments *ui;
};

#endif // DIALOGMOMENTS_H
