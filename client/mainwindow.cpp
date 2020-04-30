#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <sstream>

#include <QDebug>
#include <QMessageBox>

#include "odbc.h"

std::string MainWindow::login(const char *dataSource) {
    std::ostringstream info;
    if (odbc_login(info, dataSource, NULL, NULL)) {
        return info.str();
    }
    std::string stmt("use wechat;");
    if (exec_sql(stmt, false)) {
        //No database named wechat, create one
        if (exec_sql(std::string("CREATE DATABASE wechat"), true)) {
            logout();
            return "Error on creating database \"wechat\".";
        }
    }
    return "";
}

bool MainWindow::logout(void) {
    std::ostringstream info;
    bool fail = odbc_logout(info);
    if (fail) {
        QMessageBox::critical(this, "odbc logout error", QString(info.str().c_str()));
    }
    return fail;
}

bool MainWindow::exec_sql(const std::string& stmt, bool critical) {
    qDebug() << stmt.c_str();
    std::ostringstream err;
    bool fail = odbc_exec(err, stmt.c_str());
    if (fail) {
        if (critical) {
            qDebug() << "ERROR!";
            QMessageBox::critical(this, "Execute error!", QString(err.str().c_str()));
        } else {
            qDebug() << "Uncritical execute error:" << err.str().c_str();
        }
    }
    return fail;
}

void MainWindow::execSqlSlot(const std::string& stmt, bool critical) {
    exec_sql(stmt, critical);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    if (serverhstmt) {
        logout();
    }
    delete ui;
}
