#include "widgetmoment.h"
#include "ui_widgetmoment.h"

#include "myglobal.h"
#include "mycontent.h"

WidgetMoment::WidgetMoment(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetMoment)
{
    ui->setupUi(this);

    ui->content->setWordWrap(true);
    connect(ui->btn_comment, &QPushButton::clicked, &dialogComments, &DialogComments::HandleShow);
    connect(&dialogComments, &DialogComments::Comment, this, &WidgetMoment::slotComment);
    connect(&dialogComments, &DialogComments::CommentsReq, this, &WidgetMoment::slotCommentsReq);
}

WidgetMoment::~WidgetMoment()
{
    delete ui;
}

void WidgetMoment::HandleComments(const std::vector<CppComment>& comments) {
    dialogComments.HandleComments(comments);
}

void WidgetMoment::Set(CppMoment moment) {
    id = moment.id;
    ui->content->setText(QString(GenContentDisp(moment.sender, moment.time, moment.content).c_str()));
}

void WidgetMoment::slotComment(commentid_t reply, CppContent content) {
    emit Comment(id, reply, content);
}

void WidgetMoment::slotCommentsReq() {
    emit CommentsReq(id);
}
