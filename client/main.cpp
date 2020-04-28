#include "mainwindow.h"
#include <QApplication>

#include <QMessageBox>

MainWindow *mainWindow;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    mainWindow = new MainWindow;
    std::string errorInfo = mainWindow->login();
    if (errorInfo != "") {
        QMessageBox::critical(mainWindow, "Login error", QString(errorInfo.c_str()) +
                              "\nPlease make sure that there is an odbc data source named wechat!");
        mainWindow->deleteLater();
        return 0;
    } else {
        mainWindow->show();
        return a.exec();
    }
}
