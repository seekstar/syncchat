#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QDialog>

#include <string>
#include "sslbase.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void resetSock(ssl_socket *sock);

private:
    Ui::MainWindow *ui;
    ssl_socket *socket_;
    //QDialog *moments;
};

#endif // MAINWINDOW_H
