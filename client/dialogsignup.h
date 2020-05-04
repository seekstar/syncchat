#ifndef DIALOGSIGNUP_H
#define DIALOGSIGNUP_H

#include <QDialog>

#include <string>

#include "sslbase.h"
#include "types.h"

namespace Ui {
class DialogSignup;
}

class DialogSignup : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSignup(QWidget *parent = 0);
    ~DialogSignup();

signals:
    void sigErr(std::string msg);
    void sigDone();

public Q_SLOTS:
    void resetSock(ssl_socket *sock);

private Q_SLOTS:
    void signup();

private:
    void read_signup_reply(const boost::system::error_code& error);
    void handle_signup_reply(const boost::system::error_code& error);
    void handle_signup_reply2(const boost::system::error_code& error);

    Ui::DialogSignup *ui;
    ssl_socket *socket_;
    bool busy;

    constexpr static size_t BUFSIZE = std::max({
        sizeof(C2SHeader) + sizeof(SignupHeader) + MAX_USERNAME_LEN + MAX_PHONE_LEN,
        sizeof(S2CHeader) + sizeof(SignupReply)
    });
    uint8_t buf_[BUFSIZE];
};

#endif // DIALOGSIGNUP_H
