#include "friendinfopage.h"
#include "ui_friendinfopage.h"
#include <QDebug>

FriendInfoPage::FriendInfoPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FriendInfoPage),_user_info(nullptr)
{
    ui->setupUi(this);

    //设置光标为小手
    ui->msg_chat->setCursor(Qt::PointingHandCursor);
    ui->video_chat->setCursor(Qt::PointingHandCursor);
    ui->voice_chat->setCursor(Qt::PointingHandCursor);
    //设置三个按钮的图片
    ui->msg_chat->setStyleSheet("QPushButton {"
                                 "    border-image: url(:/res/msg_chat.png);"
                                 "}");
    ui->video_chat->setStyleSheet("QPushButton {"
                                 "    border-image: url(:/res/video_chat.png);"
                                 "}");
    ui->voice_chat->setStyleSheet("QPushButton {"
                                  "    border-image: url(:/res/voice_chat.png);"
                                  "}");
}

FriendInfoPage::~FriendInfoPage()
{
    delete ui;
}

void FriendInfoPage::SetInfo(std::shared_ptr<UserInfo> user_info)
{
    _user_info = user_info;
    // 加载图片
    QPixmap pixmap(user_info->_icon);

    // 设置图片自动缩放
    ui->icon_lb->setPixmap(pixmap.scaled(ui->icon_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_lb->setScaledContents(true);

    ui->name_lb->setText(user_info->_name);
    ui->nick_lb->setText(user_info->_nick);
    ui->bak_lb->setText(user_info->_nick);
}

void FriendInfoPage::on_msg_chat_clicked()
{
    qDebug() << "msg chat btn clicked";
    emit sig_jump_chat_item(_user_info);
}
