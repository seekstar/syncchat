#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <sstream>

#include <QDebug>
#include <QMessageBox>

#include "myodbc.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
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

void MainWindow::HandleItemClicked(QListWidgetItem *item) {
    //backup
    if (curIsUser_) {
        auto it = userChatInfo_.find(curUser_);
        assert(it != userChatInfo_.end());
        it->second.textBrowser = ui->textBrowser->toPlainText();
        it->second.textEdit = ui->textEdit->toPlainText();
        it->second.readonly = ui->textEdit->isReadOnly();
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

void MainWindow::NewFriend(userid_t userid, std::__cxx11::string username) {
    QListWidgetItem *item = new QListWidgetItem(username.c_str());
    ui->contacts->addItem(item);
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

void MainWindow::Send() {
    ui->textEdit->setReadOnly(true);
    if (curIsUser_) {
        emit SendToUser(curUser_, ui->textEdit->toPlainText().toStdString());
    } else {
        emit SendToGroup(curGroup_, ui->textEdit->toPlainText().toStdString());
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
