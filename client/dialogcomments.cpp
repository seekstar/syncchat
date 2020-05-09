#include "dialogcomments.h"
#include "ui_dialogcomments.h"

#include "widgetcomment.h"

DialogComments::DialogComments(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogComments),
    reply(0)
{
    ui->setupUi(this);
    connect(ui->btn_refresh, &QPushButton::clicked, this, &DialogComments::CommentsReq);
    connect(ui->btn_send, &QPushButton::clicked, this, &DialogComments::slotSend);

//    connect(&timer, &QTimer::timeout, [&] {
//        emit CommentsReq();
//    });
//    timer.start(100);
}

DialogComments::~DialogComments()
{
    delete ui;
}

void DialogComments::HandleShow() {
    show();
    emit CommentsReq();
}

void DialogComments::HandleComments(std::vector<CppComment> comments) {
    reply = 0;
    ui->comments->clear();
    std::sort(comments.begin(), comments.end());
    for (auto &comment : comments) {
        HandleComment(comment);
    }
}
void DialogComments::HandleComment(const CppComment& moment) {
    auto item = new QListWidgetItem;
    auto widget = new WidgetComment;
    widget->Set(moment);
    connect(widget, &WidgetComment::Reply, this, &DialogComments::HandleReply);
    item->setSizeHint(widget->sizeHint());
    ui->comments->addItem(item);
    ui->comments->setItemWidget(item, widget);
}

CppContent input2content(std::string in) {
    return CppContent(in.c_str(), in.c_str() + in.size());
}
void DialogComments::slotSend() {
    emit Comment(reply, input2content(ui->textEdit->toPlainText().toStdString()));
    ui->textEdit->clear();
    //emit CommentsReq();
}
void DialogComments::HandleReply(commentid_t id) {
    reply = id;
    ui->reply->setText(QString(("回复评论" + std::to_string(id) + ':').c_str()));
}

//void
