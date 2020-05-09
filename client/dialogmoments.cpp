#include "dialogmoments.h"
#include "ui_dialogmoments.h"

#include <QDebug>

#include "widgetmoment.h"

DialogMoments::DialogMoments(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogMoments)
{
    ui->setupUi(this);
    connect(ui->btn_edit, &QPushButton::clicked, this, &DialogMoments::sigEditMoment);
    connect(ui->btn_refresh, &QPushButton::clicked, this, &DialogMoments::MomentsReq);

//    connect(&timer, &QTimer::timeout, [&] {
//        emit MomentsReq();
//    });
//    timer.start(100);
}

DialogMoments::~DialogMoments()
{
    delete ui;
}

void DialogMoments::HandleShow() {
    show();
    emit MomentsReq();
}

void DialogMoments::HandleMoments(std::vector<CppMoment> moments) {
    qDebug() << __PRETTY_FUNCTION__ << ": Received" << moments.size() << "moments";
    ui->moments->clear();
    mmtWidget_.clear();
    std::sort(moments.begin(), moments.end());
    for (auto &moment : moments) {
        HandleMoment(moment);
    }
}
void DialogMoments::HandleMoment(CppMoment moment) {
    auto item = new QListWidgetItem;
    auto widget = new WidgetMoment;
    widget->Set(moment);
    connect(widget, &WidgetMoment::Comment, this, &DialogMoments::Comment);
    connect(widget, &WidgetMoment::CommentsReq, this, &DialogMoments::CommentsReq);
    item->setSizeHint(widget->sizeHint());
    ui->moments->addItem(item);
    ui->moments->setItemWidget(item, widget);
    mmtWidget_[moment.id] = widget;
}
void DialogMoments::HandleComments(momentid_t momentid, std::vector<CppComment> comments) {
    auto it = mmtWidget_.find(momentid);
    if (mmtWidget_.end() == it) {
        qDebug() << __PRETTY_FUNCTION__ << ": moment id" << momentid << "has no corresponding widget!";
        return;
    }
    it->second->HandleComments(comments);
}
