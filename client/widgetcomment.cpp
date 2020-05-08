#include "widgetcomment.h"
#include "ui_widgetcomment.h"

#include "mycontent.h"

WidgetComment::WidgetComment(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetComment)
{
    ui->setupUi(this);
    connect(ui->btn_reply, &QPushButton::clicked, [this] {
        emit Reply(id);
    });
}

WidgetComment::~WidgetComment()
{
    delete ui;
}

void WidgetComment::Set(CppComment comment) {
    id = comment.id;
    std::ostringstream disp;
    disp << "评论" << comment.id << ": " << NameAndId(comment.sender);
    if (comment.reply) {
        disp << " 回复评论" << comment.reply;
    }
    disp << "  " << format_ms(comment.time) << std::endl << content2str(comment.content);
    ui->content->setText(QString(disp.str().c_str()));
}
