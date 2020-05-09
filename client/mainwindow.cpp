#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <sstream>

#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>

#include <boost/lexical_cast.hpp>

#include "myglobal.h"
#include "myodbc.h"
#include "mycontent.h"

constexpr int logoIndex = 0;
constexpr int chatIndex = 1;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    myid(0),
    ui(new Ui::MainWindow),
    curIsUser_(false),
    curUser_(0),
    curGrp_(0)
{
    ui->setupUi(this);
    ui->stackedWidget_chat->setCurrentIndex(logoIndex);
    ui->textEdit->setReadOnly(true);
    //ui->textEdit->setText("点击左边的好友发送消息");
    connect(ui->actionFindByUsername, &QAction::triggered, this, &MainWindow::sigFindByUsername);
    connect(ui->actionAddFriend, &QAction::triggered, this, &MainWindow::AddFriend);
    connect(ui->btn_send, &QPushButton::clicked, this, &MainWindow::Send);
    connect(ui->chats, &QListWidget::itemClicked, this, &MainWindow::HandleItemClicked);
    connect(ui->btn_deleteFriend, &QPushButton::clicked, this, &MainWindow::HandleDeleteFriend);
    connect(ui->actionCreateGroup, &QAction::triggered, this, &MainWindow::CreateGroup);
    connect(ui->actionJoinGroup, &QAction::triggered, this, &MainWindow::JoinGroup);
    connect(ui->actionManageGroup, &QAction::triggered, this, &MainWindow::slotManageGroup);
    connect(ui->actionAllGrps, &QAction::triggered, this, &MainWindow::sigAllGrps);
    connect(ui->actionAllGrpMember, &QAction::triggered, this, &MainWindow::slotAllGrpMember);
    connect(ui->actionMoments, &QAction::triggered, this, &MainWindow::sigMoments);
    connect(ui->actionPersonalInfo, &QAction::triggered, this, &MainWindow::sigPersonalInfo);
}

void MainWindow::UpdatePrivateInfo(std::string username, std::string phone) {
    qDebug() << __PRETTY_FUNCTION__;
    myUsername = username;
    myPhone = phone;
    usernames[myid] = username;
    setWindowTitle(QString((myUsername + "(账号:" + std::to_string(myid) + ')').c_str()));
}

void MainWindow::HandleFindByUsernameReply(std::vector<userid_t> res) {
    std::ostringstream disp;
    for (userid_t userid : res) {
        disp << userid << '\n';
    }
    QMessageBox::information(this, "查询结果", QString(disp.str().c_str()));
}

void MainWindow::NewFriend(userid_t userid, std::string username) {
    usernames[userid] = username;
    QListWidgetItem *item = new QListWidgetItem((username + "(账号:" + std::to_string(userid) + ')').c_str());
    ui->chats->addItem(item);
    itemIsUser_[item] = true;
    userChatInfo[userid] = {item, "", "", false};
    itemUser_[item] = userid;
}

void MainWindow::UpdateUsername(userid_t userid, std::string username) {
    auto it = userChatInfo.find(userid);
    if (userChatInfo.end() == it) {
        qDebug() << "Warning in" << __PRETTY_FUNCTION__ << ": No such a user " << userid;
        return;
    }
    it->second.item->setText(QString((username + "(账号:" + std::to_string(userid) + ')').c_str()));
}

void MainWindow::HandlePrivateMsgTooLong(userid_t userid, CppContent content) {
    (void)content;
    auto it = usernames.find(userid);
    if (usernames.end() == it) {
        QMessageBox::information(this, "提示", QString(("你给账号为" + std::to_string(userid) + "的好友发送的消息过长").c_str()));
    } else {
        QMessageBox::information(this, "提示", QString((
            "你给好友" + it->second + "(账号为" + std::to_string(userid) + ")发送的消息过长"
        ).c_str()));
    }
    SetUserChatEditable(userid);
}
void MainWindow::HandlePrivateMsgResponse(userid_t userid, CppContent content, msgid_t msgid, msgtime_t msgtime) {
    HandlePrivateMsg(userid, myid, content, msgid, msgtime);
    ClearUserChatEdit(userid);
    SetUserChatEditable(userid);
}

void MainWindow::HandlePrivateMsg(userid_t frd, userid_t sender, CppContent content, msgid_t msgid, msgtime_t time) {
    (void)msgid;
    std::string disp = GenContentDisp(sender, time, content);
    if (curIsUser_ && curUser_ == frd) {
        ui->textBrowser->append(disp.c_str());
    } else {
        auto it = userChatInfo.find(frd);
        if (userChatInfo.end() == it) {
            qDebug() << "Warning in" << __PRETTY_FUNCTION__ << ": user" << frd << "has no corresponding chat";
            return;
        }
        it->second.textBrowser.append(disp.c_str());
    }
}

void MainWindow::NewGroup(grpid_t grpid, std::__cxx11::string grpname) {
    qDebug() << __PRETTY_FUNCTION__;
    grpnames[grpid] = grpname;
    QListWidgetItem *item = new QListWidgetItem((grpname + "(群号:" + std::to_string(grpid) + ')').c_str());
    ui->chats->addItem(item);
    itemIsUser_[item] = false;
    grpChatInfo[grpid] = {item, "", "", false};
    itemGrp_[item] = grpid;
}

void MainWindow::HandleGrpMsgResp(grpmsgid_t grpmsgid, msgtime_t msgtime, grpid_t grpid, CppContent content) {
    qDebug() << __PRETTY_FUNCTION__;
    HandleGrpMsg(grpmsgid, msgtime, myid, grpid, content);
    ClearGrpChatEdit(grpid);
    SetGrpChatEditable(grpid);
}
void MainWindow::HandleGrpMsg(grpmsgid_t grpmsgid, msgtime_t time, userid_t sender, grpid_t grpid, CppContent content) {
    qDebug() << __PRETTY_FUNCTION__;
    (void)grpmsgid;
    std::string disp = GenContentDisp(sender, time, content);
    if (!curIsUser_ && curGrp_ == grpid) {
        ui->textBrowser->append(disp.c_str());
    } else {
        auto it = grpChatInfo.find(grpid);
        if (userChatInfo.end() == it) {
            qDebug() << "Warning in" << __PRETTY_FUNCTION__ << ": group" << grpid << "has no corresponding chat";
            return;
        }
        it->second.textBrowser.append(disp.c_str());
    }
}

void MainWindow::HandleItemClicked(QListWidgetItem *item) {
    qDebug() << item->text() << "clicked";
    ui->stackedWidget_chat->setCurrentIndex(chatIndex);
    //backup
    if (curIsUser_) {
        if (curUser_) {
            auto it = userChatInfo.find(curUser_);
            assert(it != userChatInfo.end());
            it->second.textBrowser = ui->textBrowser->toPlainText();
            it->second.textEdit = ui->textEdit->toPlainText();
            it->second.readonly = ui->textEdit->isReadOnly();
        }
    } else {
        if (curGrp_) {
            auto it = grpChatInfo.find(curGrp_);
            assert(it != grpChatInfo.end());
            it->second.textBrowser = ui->textBrowser->toPlainText();
            it->second.textBrowser = ui->textEdit->toPlainText();
            it->second.readonly = ui->textEdit->isReadOnly();
        }
    }
    curIsUser_ = itemIsUser_[item];
    if (curIsUser_) {
        curUser_ = itemUser_[item];
        auto it = userChatInfo.find(curUser_);
        if (userChatInfo.end() == it) {
            qDebug() << "Error in" << __PRETTY_FUNCTION__ << ": item" << item << "has no corresponding user";
            return;
        }
        ui->textBrowser->setPlainText(it->second.textBrowser);
        ui->textEdit->setPlainText(it->second.textEdit);
        ui->textEdit->setReadOnly(it->second.readonly);
        ui->btn_send->setEnabled(!it->second.readonly);
    } else {
        curGrp_ = itemGrp_[item];
        auto it = grpChatInfo.find(curGrp_);
        if (userChatInfo.end() == it) {
            qDebug() << "Error in" << __PRETTY_FUNCTION__ << ": item" << item << "has no corresponding group";
            return;
        }
        ui->textBrowser->setPlainText(it->second.textBrowser);
        ui->textEdit->setPlainText(it->second.textEdit);
        ui->textEdit->setReadOnly(it->second.readonly);
        ui->btn_send->setEnabled(!it->second.readonly);
    }
}

CppContent MainWindow::GetContentByInput(std::string in) {
    return CppContent(in.c_str(), in.c_str() + in.length());
}

void MainWindow::Send() {
    ui->textEdit->setReadOnly(true);
    CppContent content = GetContentByInput(ui->textEdit->toPlainText().toStdString());
    if (curIsUser_) {
        emit SendToUser(curUser_, content);
    } else {
        emit SendToGroup(curGrp_, content);
    }
}

void MainWindow::HandleDeleteFriend() {
    if (curIsUser_) {
        emit sigDeleteFriend(curUser_);
        DeleteFriend(curUser_);
    } else {
        QMessageBox::information(this, "提示", "暂未实现退群功能");
    }
}
void MainWindow::DeleteFriend(userid_t userid) {
    delete ui->chats->takeItem(ui->chats->row(userChatInfo[userid].item));
    userChatInfo.erase(userid);
    if (curIsUser_ && curUser_ == userid) {
        ui->stackedWidget_chat->setCurrentIndex(logoIndex);
    }
    QMessageBox::information(this, "提示", QString(("成功与用户" + std::to_string(userid) + "解除好友关系").c_str()));
}

void MainWindow::slotManageGroup() {
    std::string grpidIn = QInputDialog::getText(this, "请输入群号", "群号：").toStdString();
    grpid_t grpid;
    try {
        grpid = boost::lexical_cast<grpid_t>(grpidIn.c_str(), grpidIn.size());
    } catch (boost::bad_lexical_cast& e) {
        QMessageBox::information(this, "提示", QString("输入错误\n") + e.what());
        return;
    }
    std::string useridIn = QInputDialog::getText(this, "请输入新群主账号", "账号：").toStdString();
    userid_t userid;
    try {
        userid = boost::lexical_cast<userid_t>(useridIn.c_str(), useridIn.size());
    } catch (boost::bad_lexical_cast& e) {
        QMessageBox::information(this, "提示", QString("输入错误\n") + e.what());
        return;
    }
    emit ChangeGrpOwner(grpid, userid);
}

void MainWindow::slotAllGrpMember() {
    std::string grpidIn = QInputDialog::getText(this, "请输入群号", "群号：").toStdString();
    try {
        grpid_t grpid = boost::lexical_cast<grpid_t>(grpidIn.c_str(), grpidIn.size());
        emit sigAllGrpMember(grpid);
    } catch (boost::bad_lexical_cast& e) {
        QMessageBox::information(this, "提示", QString("输入错误\n") + e.what());
    }
}

void MainWindow::SetUserChatEditable(userid_t userid) {
    if (curIsUser_ && curUser_ == userid) {
        ui->textEdit->setReadOnly(false);
    } else {
        auto it = userChatInfo.find(userid);
        if (userChatInfo.end() == it) {
            qDebug() << "Error in" << __PRETTY_FUNCTION__ << ": User " << userid << " has no corresponding chat";
            return;
        }
        it->second.readonly = false;
    }
}
void MainWindow::ClearUserChatEdit(userid_t userid) {
    if (curIsUser_ && curUser_ == userid) {
        ui->textEdit->clear();
    } else {
        auto it = userChatInfo.find(userid);
        if (userChatInfo.end() == it) {
            qDebug() << "Error in" << __PRETTY_FUNCTION__ << ": User " << userid << " has no corresponding chat";
            return;
        }
        it->second.textEdit = "";
    }
}

void MainWindow::SetGrpChatEditable(userid_t grpid) {
    if (!curIsUser_ && curGrp_ == grpid) {
        ui->textEdit->setReadOnly(false);
    } else {
        auto it = grpChatInfo.find(grpid);
        if (grpChatInfo.end() == it) {
            qDebug() << "Error in" << __PRETTY_FUNCTION__ << ": Group " << grpid << " has no corresponding chat";
            return;
        }
        it->second.readonly = false;
    }
}
void MainWindow::ClearGrpChatEdit(grpid_t grpid) {
    if (!curIsUser_ && curGrp_ == grpid) {
        ui->textEdit->clear();
    } else {
        auto it = grpChatInfo.find(grpid);
        if (grpChatInfo.end() == it) {
            qDebug() << "Error in" << __PRETTY_FUNCTION__ << ": Group " << grpid << " has no corresponding chat";
            return;
        }
        it->second.textEdit.clear();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
