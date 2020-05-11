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
    void HandleFindByUsernameReply(std::vector<userid_t> res);

    void HandlePrivateMsgTooLong(userid_t userid, CppContent content);
    void HandlePrivateMsgResponse(userid_t userid, CppContent content, msgid_t msgid, msgtime_t msgtime);
    void HandlePrivateMsg(userid_t frd, userid_t sender, CppContent content, msgid_t msgid, msgtime_t time);
    void HandleRawPrivateMsg(msgid_t msgid, msgtime_t time, userid_t sender, userid_t touser, CppContent content);

    void NewGroup(grpid_t grpid, std::string grpname);
    void UpdateGrpname(grpid_t grpid, std::string grpname);
    void HandleGrpMsgResp(grpmsgid_t grpmsgid, msgtime_t msgtime, grpid_t grpid, CppContent content);
    void HandleGrpMsg(grpmsgid_t grpmsgid, msgtime_t msgtime, userid_t sender, grpid_t grpid, CppContent content);

    userid_t myid;
    std::string myUsername, myPhone;
    std::unordered_map<userid_t, ChatInfo> userChatInfo;
    std::unordered_map<grpid_t, ChatInfo> grpChatInfo;

signals:
    void sigFindByUsername();
    void AddFriend();
    void sigDeleteFriend(userid_t);
    void SendToUser(userid_t user, CppContent content);
    void SendToGroup(grpid_t group, CppContent content);
    void CreateGroup();
    void JoinGroup();
    void ChangeGrpOwner(grpid_t, userid_t);
    void sigAllGrpMember(grpid_t);
    void sigMoments();
    void sigPersonalInfo();

private slots:
    void HandleItemClicked(QListWidgetItem *item);
    void Send();
    void HandleDeleteFriend();
    void slotManageGroup();
    void slotAllGrpMember();

private:
    CppContent GetContentByInput(std::string in);
    void SetUserChatEditable(userid_t userid);
    void ClearUserChatEdit(userid_t userid);
    void SetGrpChatEditable(grpid_t grpid);
    void ClearGrpChatEdit(grpid_t grpid);

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
