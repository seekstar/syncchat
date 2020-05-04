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

MainWindow::~MainWindow()
{
    delete ui;
}
