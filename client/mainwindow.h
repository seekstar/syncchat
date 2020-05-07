#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>

#include <unordered_map>
#include <string>
#include "sslbase.h"
#include "types.h"
#include "clienttypes.h"

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
    void UpdatePrivateInfo(std::string username, std::string phone);
    void NewFriend(userid_t userid, std::string username);
    void DeleteFriend(userid_t userid);
    void UpdateUsername(userid_t userid, std::string username);
    void HandlePrivateMsgTooLong(userid_t userid, msgcontent_t content);
    void HandlePrivateMsgResponse(userid_t userid, msgcontent_t content, msgid_t msgid, msgtime_t msgtime);
    void HandlePrivateMsg(userid_t frd, userid_t sender, msgcontent_t content, msgid_t msgid, msgtime_t time);
    void NewGroup(grpid_t grpid, std::string grpname);

    userid_t myid;
    std::string myUsername, myPhone;
    std::unordered_map<userid_t, std::string> usernames;
    std::unordered_map<userid_t, ChatInfo> userChatInfo;
    std::unordered_map<grpid_t, ChatInfo> grpChatInfo;
    std::unordered_map<grpid_t, std::string> grpnames;

signals:
    void AddFriend();
    void sigDeleteFriend(userid_t);
    void SendToUser(userid_t user, msgcontent_t content);
    void SendToGroup(grpid_t group, msgcontent_t content);
    void CreateGroup();
    void JoinGroup();

private Q_SLOTS:
    void HandleItemClicked(QListWidgetItem *item);
    void Send();
    void HandleDeleteFriend();

private:
    std::string content2str(msgcontent_t content);
    msgcontent_t GetContentByInput(std::string in);
    void SetUserChatEditable(userid_t userid);
    void ClearUserChatEdit(userid_t userid);

    Ui::MainWindow *ui;

    bool curIsUser_;
    userid_t curUser_;
    grpid_t curGrp_;
    //textBrowser, textEdit
    std::unordered_map<QListWidgetItem *, bool> itemIsUser_;
    std::unordered_map<QListWidgetItem *, userid_t> itemUser_;
    std::unordered_map<QListWidgetItem *, grpid_t> itemGrp_;
};

#endif // MAINWINDOW_H
