#ifndef WINLOGIN_H
#define WINLOGIN_H

#include <QWidget>

namespace Ui {
class WinLogin;
}

class WinLogin : public QWidget
{
    Q_OBJECT

public:
    explicit WinLogin(QWidget *parent = 0);
    ~WinLogin();

public Q_SLOTS:
    void login();

private:
    void read_login_reply();

    Ui::WinLogin *ui;
    bool busy;

    const static int BUFSIZE = 4096;
    char buf_[BUFSIZE];
};

#endif // WINLOGIN_H
