#ifndef DIALOGRECONNECT_H
#define DIALOGRECONNECT_H

#include <QDialog>

namespace Ui {
class DialogReconnect;
}

class DialogReconnect : public QDialog
{
    Q_OBJECT

public:
    explicit DialogReconnect(QWidget *parent = 0);
    ~DialogReconnect();

private:
    Ui::DialogReconnect *ui;
};

#endif // DIALOGRECONNECT_H
