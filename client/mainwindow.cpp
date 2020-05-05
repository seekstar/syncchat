#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <sstream>

#include <QDebug>
#include <QMessageBox>

#include "myodbc.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->actionAddFriend, &QAction::triggered, this, &MainWindow::AddFriend);
}

void MainWindow::NewFriend(userid_t userid, std::__cxx11::string username) {
    if ("" == username) {
        username = std::to_string(userid);
    }
    QListWidgetItem *item = new QListWidgetItem(username.c_str());
    ui->contacts->addItem(item);
    userListItem_[userid] = item;
}
void MainWindow::UpdateUsername(userid_t userid, std::string username) {
    auto it = userListItem_.find(userid);
    if (userListItem_.end() == it) {
        qDebug() << "Warning in" << __PRETTY_FUNCTION__ << ": No such a user " << userid;
        return;
    }
    it->second->setText(QString(username.c_str()));
}

MainWindow::~MainWindow()
{
    delete ui;
}
