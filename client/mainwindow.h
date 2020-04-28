#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <string>

#include <QDialog>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    std::string login(void);
    bool exec_sql(const std::string& stmt, bool critical);
    bool logout(void);

private Q_SLOTS:
    void execSqlSlot(const std::string& stmt, bool critical);

private:
    Ui::MainWindow *ui;
    QDialog *moments;
};

#endif // MAINWINDOW_H
