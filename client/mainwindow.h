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

struct ChatInfo {
    QListWidgetItem *item;
    QString textBrowser;
    QString textEdit;
    bool readonly;
};

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
    void SendToUser(userid_t user, std::string content);
    void SendToGroup(groupid_t group, std::string content);

private:
    void HandleItemClicked(QListWidgetItem *item);
    void Send();

    Ui::MainWindow *ui;

    bool curIsUser_;
    userid_t curUser_;
    groupid_t curGroup_;
    //textBrowser, textEdit
    std::unordered_map<QListWidgetItem *, bool> itemIsUser_;
    std::unordered_map<userid_t, ChatInfo> userChatInfo_;
    std::unordered_map<QListWidgetItem *, userid_t> itemUser_;
    std::unordered_map<groupid_t, ChatInfo> groupChatInfo_;
    std::unordered_map<QListWidgetItem *, groupid_t> itemGroup_;
};

#endif // MAINWINDOW_H
