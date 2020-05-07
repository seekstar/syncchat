#ifndef DIALOGCREATEGROUP_H
#define DIALOGCREATEGROUP_H

#include <QDialog>

namespace Ui {
class DialogCreateGroup;
}

class DialogCreateGroup : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateGroup(QWidget *parent = 0);
    ~DialogCreateGroup();

signals:
    void CreateGroup(std::string groupname);

private slots:
    void slotCreateGroup();

private:
    Ui::DialogCreateGroup *ui;
};

#endif // DIALOGCREATEGROUP_H
