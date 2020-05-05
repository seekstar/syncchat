#include "mainmanager.h"

#include <QMessageBox>
#include <QDebug>

#include "myodbc.h"
#include "escape.h"

MainManager::MainManager(const char *ip, const char *port, QObject *parent)
    : QObject(parent),
      sslManager(ip, port),
      myid(0)
{
    qDebug() << "The thread id of the main thread is " << QThread::currentThreadId();
    //sslManager
    connect(&sslManager, &SslManager::loginFirst, [this] {
        QMessageBox::information(&mainWindow, "提示", "请先登录");
    });
    //Ignore it
//    connect(&sslManager, &SslManager::alreadyLogined, [this] {
//        QMessageBox::information(this, "提示", "您已登录");
//    });
    connect(&sslManager, &SslManager::usernameTooLong, [this] {
        QMessageBox::information(&dialogSignup, "提示", "昵称过长");
    });
    connect(&sslManager, &SslManager::phoneTooLong, [this] {
        QMessageBox::information(&dialogSignup, "提示", "手机号过长");
    });
    connect(&sslManager, &SslManager::wrongPassword, [this] {
        QMessageBox::information(&winLogin, "提示", "密码错误");
    });
    connect(&sslManager, &SslManager::alreadyFriends, [this] {
        QMessageBox::information(&dialogAddFriend, "提示", "已经是好友");
    });
    connect(&sslManager, &SslManager::addFriendSent, [this] {
        QMessageBox::information(&dialogAddFriend, "提示", "申请已发送，请等待对方同意");
    });

    //dialogReconnect and sslManager
    connect(&sslManager, &SslManager::sigErr, &dialogReconnect, &DialogReconnect::ShowErr);
    connect(&dialogReconnect, &DialogReconnect::sigRetry, &sslManager, &SslManager::sslconn);
    connect(&sslManager, &SslManager::sigDone, &dialogReconnect, &DialogReconnect::close);
    //dialogSignup
    connect(&winLogin, &WinLogin::signup, &dialogSignup, &DialogSignup::show);
    connect(&dialogSignup, &DialogSignup::signup, &sslManager, &SslManager::signup);
    connect(&sslManager, &SslManager::signupDone, &dialogSignup, &DialogSignup::signupRes);
    //winLogin
    connect(&winLogin, &WinLogin::login, &sslManager, &SslManager::login);
    connect(&sslManager, &SslManager::loginDone, this, &MainManager::HandleLoginDone);
    //dialogAddFriend
    connect(&mainWindow, &MainWindow::AddFriend, &dialogAddFriend, &DialogAddFriend::show);
    connect(&dialogAddFriend, &DialogAddFriend::AddFriend, &sslManager, &SslManager::AddFriend);
    connect(&sslManager, &SslManager::addFriendReq, this, &MainManager::HandleAddFriendReq);
    connect(this, &MainManager::replyAddFriend, &sslManager, &SslManager::ReplyAddFriend);
    connect(&sslManager, &SslManager::addFriendReply, this, &MainManager::HandleAddFriendReply);
    connect(this, &MainManager::UserPublicInfoReq, &sslManager, &SslManager::UserPublicInfoReq);
    connect(&sslManager, &SslManager::UserPublicInfoReply, this, &MainManager::HandleUserPublicInfoReply);

    sslManager.start();
    winLogin.show();
}

MainManager::~MainManager() {
    myodbcLogout();
    //sslManager.stop_io_service();
    sslManager.quit();
    sslManager.wait();
}

void MainManager::HandleLoginDone(userid_t userid) {
    if (myid) {
        qDebug() << "Duplicate login done: " << userid;
        return;
    }
    myid = userid;
    winLogin.close();
    mainWindow.setWindowTitle(QString(std::to_string(myid).c_str()));
    mainWindow.show();
    myodbcLogin(("Driver=SQLite3;Database=syncchatclient" + std::to_string(myid) + ".db").c_str());
    if (exec_sql("SELECT userid, username FROM friends;", true)) {
        return;
    }
    SQLLEN idLen, nameLen;
    char username[MAX_USERNAME_LEN];
    SQLBindCol(hstmt, 1, SQL_C_UBIGINT, &userid, sizeof(userid), &idLen);
    SQLBindCol(hstmt, 2, SQL_C_CHAR, username, sizeof(username), &nameLen);
    while (SQL_NO_DATA != SQLFetch(hstmt)) {
        assert(SQL_NULL_DATA != idLen);
        std::string displayName;
        if (SQL_NULL_DATA == nameLen) {
            displayName = std::to_string(userid);
        } else {
            displayName = std::string(username, nameLen);
        }
        mainWindow.NewFriend(userid, displayName);
    }
}

void MainManager::HandleAddFriendReq(userid_t userid, std::__cxx11::string username) {
    std::string msg = "账号为" + std::to_string(userid) + "\n昵称为" + username + "\n同意吗？";
    bool reply = (QMessageBox::Yes == QMessageBox::question(
        NULL, "加好友请求", QString(msg.c_str()), QMessageBox::Yes | QMessageBox::No
    ));
    if (reply) {
        exec_sql("INSERT INTO friends(userid, username) VALUES(" +
                 std::to_string(userid) + ",\"" + escape(username) + "\");", true);
        usernames[userid] = username;
        mainWindow.NewFriend(userid, username);
    }
    emit replyAddFriend(userid, reply);
}

void MainManager::HandleAddFriendReply(userid_t userid, bool reply) {
    if (!reply) {
        QMessageBox::information(NULL, "提示", "用户" + QString(std::to_string(userid).c_str()) + "拒绝了您的好友申请");
        return;
    }
    auto it = usernames.find(userid);
    if (usernames.end() == it) {
        emit UserPublicInfoReq(userid);
        exec_sql("INSERT INTO friends(userid) VALUES(" + std::to_string(userid) + ");", true);
        mainWindow.NewFriend(userid, std::to_string(userid));
    } else {
        exec_sql("INSERT INTO friends(userid, username) VALUES(" +
                 std::to_string(userid) + ",\"" + escape(it->second) + "\");", true);
        mainWindow.NewFriend(userid, it->second);
    }
    QMessageBox::information(NULL, "提示", "您与" + QString(std::to_string(userid).c_str()) + "成为好友了");
}
void MainManager::HandleUserPublicInfoReply(userid_t userid, std::string username) {
    exec_sql("UPDATE friends SET username = \"" + escape(username) + "\" WHERE userid = " + std::to_string(userid) + ';', true);
    usernames[userid] = username;
    qDebug() << "userid = " << userid << ", username = " << username.c_str();
    mainWindow.UpdateUsername(userid, username);
}
