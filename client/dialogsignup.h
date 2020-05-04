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
    void signup(std::vector<uint8_t> content);

public Q_SLOTS:
    void signupRes(userid_t userid);

private Q_SLOTS:
    void slotSignup();

private:
    Ui::DialogSignup *ui;

//    constexpr static size_t BUFSIZE = std::max({
//        sizeof(C2SHeader) + sizeof(SignupHeader) + MAX_USERNAME_LEN + MAX_PHONE_LEN,
//        sizeof(S2CHeader) + sizeof(SignupReply)
//    });
//    uint8_t buf_[BUFSIZE];
};

#endif // DIALOGSIGNUP_H
