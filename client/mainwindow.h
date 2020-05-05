#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>

#include <unordered_map>
#include <string>
#include "sslbase.h"
#include "types.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void NewFriend(userid_t userid, std::string username);
    void UpdateUsername(userid_t userid, std::string username);

signals:
    void AddFriend();

private:
    Ui::MainWindow *ui;

    std::unordered_map<userid_t, QListWidgetItem *> userListItem_;
};

#endif // MAINWINDOW_H
