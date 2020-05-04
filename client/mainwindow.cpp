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
}

void MainWindow::resetSock(ssl_socket *sock) {
    socket_ = sock;
}

MainWindow::~MainWindow()
{
    delete ui;
}
