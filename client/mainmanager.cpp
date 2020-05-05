#include "mainmanager.h"

#include <QMessageBox>
#include <QDebug>

#include "myodbc.h"
#include "escape.h"

MainManager::MainManager(const char *ip, const char *port, QObject *parent)
    : QObject(parent),
      sslManager(ip, port)
{
    myodbcLogin();

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
    connect(&sslManager, &SslManager::loginDone, &winLogin, &WinLogin::close);
    //dialogAddFriend
    connect(&sslManager, &SslManager::loginDone, &mainWindow, &MainWindow::show);
    connect(&mainWindow, &MainWindow::AddFriend, &dialogAddFriend, &DialogAddFriend::show);
    connect(&dialogAddFriend, &DialogAddFriend::AddFriend, &sslManager, &SslManager::AddFriend);
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

void MainManager::HandleAddFriendReply(userid_t userid, bool reply) {
    if (reply) {
        QMessageBox::information(NULL, "提示", "您与" + QString(std::to_string(userid).c_str()) + "成为好友了");
    } else {
        QMessageBox::information(NULL, "提示", "用户" + QString(std::to_string(userid).c_str()) + "拒绝了您的好友申请");
    }
    auto it = usernames.find(userid);
    if (usernames.end() == it) {
        emit UserPublicInfoReq(userid);
        exec_sql("INSERT INTO friends(userid) VALUES(" + std::to_string(userid) + ");", true);
    } else {
        exec_sql("INSERT INTO friends(userid, username) VALUES(" +
                 std::to_string(userid) + ",\"" + escape(it->second) + "\");", true);
    }
}
void MainManager::HandleUserPublicInfoReply(userid_t userid, std::string username) {
    exec_sql("UPDATE friends SET username = \"" + escape(username) + "\" WHERE userid = " + std::to_string(userid) + ';', true);
    usernames[userid] = username;
    qDebug() << "userid = " << userid << ", username = " << username.c_str();
}
