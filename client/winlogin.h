#ifndef WINLOGIN_H
#define WINLOGIN_H

#include <QWidget>

#include "dialogsignup.h"

#include "types.h"
#include "sslbase.h"

namespace Ui {
class WinLogin;
}

//Do not consider push from server
class WinLogin : public QWidget
{
    Q_OBJECT

public:
    explicit WinLogin(QWidget *parent = 0);
    ~WinLogin();

signals:
    void sigErr(std::string msg);
    void sigDone(void);

public Q_SLOTS:
    void resetSock(ssl_socket *sock);
    void login();

private:
    void handle_login_reply(const boost::system::error_code& error);
    void read_login_reply(const boost::system::error_code& error);

    Ui::WinLogin *ui;
    ssl_socket *socket_;
    bool busy;  //Prevent dialogSignup from showing and prevent relogin

    DialogSignup dialogSignup;

    const static int BUFSIZE = std::max({
        sizeof(C2SHeader) + sizeof(LoginInfo),
        sizeof(C2SHeader)
    });
    char buf_[BUFSIZE];
};

#endif // WINLOGIN_H
