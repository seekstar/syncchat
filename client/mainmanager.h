#ifndef MAINMANAGER_H
#define MAINMANAGER_H

#include <QObject>
#include <QDebug>

#include "dialogreconnect.h"
#include "dialogfindbyusername.h"
#include "dialogaddfriend.h"
#include "sslmanager.h"
#include "winlogin.h"
#include "mainwindow.h"
#include "dialogcreategroup.h"
#include "dialogjoingroup.h"
#include "dialogmoments.h"
#include "dialogeditmoment.h"

#include <unordered_map>

#include "clienttypes.h"

class MainManager : public QObject
{
    Q_OBJECT
public:
    explicit MainManager(const char *ip, const char *port, QObject *parent = 0);
    ~MainManager();

signals:
    void replyAddFriend(userid_t userid, bool reply);
    void UserPrivateInfoReq();
    void UserPublicInfoReq(userid_t);
    void AllFriendsReq();
    //void AllGrpsReq();

public slots:

private slots:
    void HandleLoginDone(userid_t userid);
    void HandleAddFriendReq(userid_t userid, std::string username);
    void HandleAddFriendReply(userid_t userid, bool reply);
    void HandleNewFriend(userid_t userid);
    void HandleUserPublicInfoReply(userid_t userid, std::string username);
    void HandleFriends(std::vector<userid_t> friends);
    void DeleteFriend(userid_t);

    void HandlePrivateMsgResponse(userid_t userid, msgcontent_t content, msgid_t msgid, msgtime_t msgtime);
    void HandleReceivedPrivateMsg(userid_t userid, msgcontent_t content, msgid_t msgid, msgtime_t msgtime);

    void HandleNewGroup(grpid_t grpid, std::string grpname);
    //void HandleJoinGroup(grpid_t grpid);
    void HandleGrpMsgResp(grpmsgid_t grpmsgid, msgtime_t msgtime, grpid_t grpid, msgcontent_t content);
    void HandleReceivedGrpMsg(grpmsgid_t grpmsgid, msgtime_t msgtime, userid_t sender, grpid_t grpid, msgcontent_t content);

private:
    bool WritePrivateMsgToDB(msgid_t msgid, msgtime_t msgtime, userid_t sender, userid_t touser, msgcontent_t content);
    bool WriteGrpMsgToDB(grpmsgid_t grpmsgid, msgtime_t msgtime, userid_t sender, grpid_t grpid, msgcontent_t content);

    SslManager sslManager;
    DialogReconnect dialogReconnect;

    DialogSignup dialogSignup;
    WinLogin winLogin;
    MainWindow mainWindow;
    DialogFindByUsername dialogFindByUsername;
    DialogAddFriend dialogAddFriend;
    DialogCreateGroup dialogCreateGroup;
    DialogJoinGroup dialogJoinGroup;
    DialogMoments dialogMoments;
    DialogEditMoment dialogEditMoment;
};

#endif // MAINMANAGER_H
