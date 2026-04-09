#include "friendlabel.h"
#include "ui_friendlabel.h"
#include <QDebug>

FriendLabel::FriendLabel(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::FriendLabel)
{
    ui->setupUi(this);
    ui->close_btn->setCursor(Qt::PointingHandCursor);
    ui->close_btn->setStyleSheet("QPushButton {"
                                 "    border-image: url(:/res/tipclose.png);"
                                 "}");
    connect(ui->close_btn, &QPushButton::clicked, this, &FriendLabel::slot_close);
}

FriendLabel::~FriendLabel()
{
    delete ui;
}

void FriendLabel::SetText(QString text)
{
    _text = text;
    ui->tip_lb->setText(_text);
    ui->tip_lb->adjustSize();

    this->setFixedWidth(ui->tip_lb->width() + ui->close_btn->width() + 20);
    this->setFixedHeight(ui->tip_lb->height() + 2);

    _width = this->width();
    _height = this->height();
}

int FriendLabel::Width()
{
    return _width;
}

int FriendLabel::Height()
{
    return _height;
}

QString FriendLabel::Text()
{
    return _text;
}

void FriendLabel::slot_close()
{
    emit sig_close(_text);
}
