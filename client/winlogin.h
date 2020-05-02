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
    void handle_error(const boost::system::error_code& error, const char *where);
    void read_login_reply(const boost::system::error_code& error);

    Ui::WinLogin *ui;
    bool busy;

    const static int BUFSIZE = std::max({
        sizeof(C2SHeader) + sizeof(LoginInfo)
    });
    char buf_[BUFSIZE];
};

#endif // WINLOGIN_H
