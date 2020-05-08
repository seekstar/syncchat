#ifndef DIALOGFINDBYUSERNAME_H
#define DIALOGFINDBYUSERNAME_H

#include <QDialog>

namespace Ui {
class DialogFindByUsername;
}

class DialogFindByUsername : public QDialog
{
    Q_OBJECT

public:
    explicit DialogFindByUsername(QWidget *parent = 0);
    ~DialogFindByUsername();

signals:
    void FindByUsername(std::string username);

private slots:
    void slotFindByUsername();

private:
    Ui::DialogFindByUsername *ui;
};

#endif // DIALOGFINDBYUSERNAME_H
