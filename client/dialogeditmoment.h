#ifndef DIALOGEDITMOMENT_H
#define DIALOGEDITMOMENT_H

#include <QDialog>

#include "types.h"

namespace Ui {
class DialogEditMoment;
}

class DialogEditMoment : public QDialog
{
    Q_OBJECT

public:
    explicit DialogEditMoment(QWidget *parent = 0);
    ~DialogEditMoment();

signals:
    void SendMoment(CppContent content);

private slots:
    void slotSendMoment();

private:
    Ui::DialogEditMoment *ui;
};

#endif // DIALOGEDITMOMENT_H
