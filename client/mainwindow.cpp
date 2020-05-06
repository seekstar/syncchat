#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <sstream>

#include <QDebug>
#include <QMessageBox>

#include "myodbc.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    myid(0),
    ui(new Ui::MainWindow),
    curIsUser_(false),
    curUser_(0),
    curGroup_(0)
{
    ui->setupUi(this);
    connect(ui->actionAddFriend, &QAction::triggered, this, &MainWindow::AddFriend);
    connect(ui->btn_send, &QPushButton::clicked, this, &MainWindow::Send);
    connect(ui->chats, &QListWidget::itemClicked, this, &MainWindow::HandleItemClicked);
}

void MainWindow::UpdatePrivateInfo(std::string username, std::string phone) {
    qDebug() << __PRETTY_FUNCTION__;
    myUsername = username;
    myPhone = phone;
    usernames[myid] = username;
    setWindowTitle(QString(myUsername.c_str()));
}

void MainWindow::NewFriend(userid_t userid, std::string username) {
    usernames[userid] = username;
    QListWidgetItem *item = new QListWidgetItem(username.c_str());
    ui->chats->addItem(item);
    itemIsUser_[item] = true;
    userChatInfo_[userid] = {item, "", "", false};
    itemUser_[item] = userid;
}

void MainWindow::UpdateUsername(userid_t userid, std::string username) {
    auto it = userChatInfo_.find(userid);
    if (userChatInfo_.end() == it) {
        qDebug() << "Warning in" << __PRETTY_FUNCTION__ << ": No such a user " << userid;
        return;
    }
    it->second.item->setText(QString(username.c_str()));
}

void MainWindow::HandlePrivateMsgTooLong(userid_t userid, msgcontent_t content) {
    (void)content;
    auto it = usernames.find(userid);
    if (usernames.end() == it) {
        QMessageBox::information(this, "提示", QString(("你给账号为" + std::to_string(userid) + "的好友发送的消息过长").c_str()));
    } else {
        QMessageBox::information(this, "提示", QString((
            "你给好友" + it->second + "(账号为" + std::to_string(userid) + ")发送的消息过长"
        ).c_str()));
    }
}

std::string MainWindow::content2str(msgcontent_t content) {
    return std::string((const char*)content.data(), content.size());
}
void MainWindow::HandlePrivateMsg(userid_t frd, userid_t sender, msgcontent_t content, msgid_t msgid, msgtime_t time) {
    (void)msgid;
    time_t tt = time / 1000;
    std::tm *now = std::localtime(&tt);
    std::ostringstream disp;
    disp << '\n';
    auto it = usernames.find(sender);
    if (usernames.end() == it) {
        disp << sender;
    } else {
        disp << it->second;
    }
    disp << "  " << (now->tm_year + 1900) << '-' << now->tm_mon << '-' << now->tm_mday << ' '
         << now->tm_hour << ':' << now->tm_min << ':' << now->tm_sec << std::endl << content2str(content);
    if (curIsUser_ && curUser_ == frd) {
        ui->textBrowser->append(disp.str().c_str());
    } else {
        auto it = userChatInfo_.find(frd);
        if (userChatInfo_.end() == it) {
            qDebug() << "Warning in" << __PRETTY_FUNCTION__ << ": user" << frd << "has no corresponding chat";
            return;
        }
        it->second.textBrowser.append(disp.str().c_str());
    }
}

void MainWindow::HandleItemClicked(QListWidgetItem *item) {
    qDebug() << item->text() << "clicked";
    //backup
    if (curIsUser_) {
        if (curUser_) {
            auto it = userChatInfo_.find(curUser_);
            assert(it != userChatInfo_.end());
            it->second.textBrowser = ui->textBrowser->toPlainText();
            it->second.textEdit = ui->textEdit->toPlainText();
            it->second.readonly = ui->textEdit->isReadOnly();
        }
    } else {
//        curGroup_ = itemGroup_[item];
//        auto it = groupChatInfo_.find(curGroup_);
//        if (groupChatInfo_.end() == it) {
//            qDebug() << "Error in" << __PRETTY_FUNCTION__ << ": item" << item << "has no corresponding group";
//            return;
//        }
//        it->second.textBrowser = ui->textBrowser->toPlainText();
//        it->second.textEdit = ui->textEdit->toPlainText();
    }
    curIsUser_ = itemIsUser_[item];
    if (curIsUser_) {
        curUser_ = itemUser_[item];
        auto it = userChatInfo_.find(curUser_);
        if (userChatInfo_.end() == it) {
            qDebug() << "Error in" << __PRETTY_FUNCTION__ << ": item" << item << "has no corresponding user";
            return;
        }
        ui->textBrowser->setPlainText(it->second.textBrowser);
        ui->textEdit->setPlainText(it->second.textEdit);
        ui->textEdit->setReadOnly(it->second.readonly);
    } else {

    }
}

msgcontent_t MainWindow::GetContentByInput(std::string in) {
    return msgcontent_t(in.c_str(), in.c_str() + in.length());
}

void MainWindow::Send() {
    ui->textEdit->setReadOnly(true);
    msgcontent_t content = GetContentByInput(ui->textEdit->toPlainText().toStdString());
    if (curIsUser_) {
        emit SendToUser(curUser_, content);
    } else {
        emit SendToGroup(curGroup_, content);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
