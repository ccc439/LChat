#ifndef SIDECONTACTBTN_H
#define SIDECONTACTBTN_H

#include <QPushButton>

class SideContactBtn:public QPushButton
{
    Q_OBJECT
public:
    explicit SideContactBtn(QWidget *parent = nullptr);
    //按下状态：切换界面
    void mousePressEvent(QMouseEvent *e) override;

    //只为了屏蔽setCheckable(false);
    void nextCheckState() override;
signals:
    //发送信号通知：side_contact_btn被按下
    void sig_press_contact();
public slots:
    //当side_chat_btn被按下时，调整状态
    void slot_press_chat();
};
#endif // SIDECONTACTBTN_H
