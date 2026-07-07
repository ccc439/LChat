#ifndef EYEICONBTN_H
#define EYEICONBTN_H
#include <QPushButton>

//普通状态->悬停状态->按下状态：密码可见->（释放）悬停状态：密码不可见->普通状态
class EyeIconBtn:public QPushButton
{
    Q_OBJECT
public:
    explicit EyeIconBtn(QWidget *parent = nullptr);
    //按下状态：密码可见
    void mousePressEvent(QMouseEvent *e) override;
    //（释放）悬停状态：密码不可见
    void mouseReleaseEvent(QMouseEvent *e) override;
signals:
    //发送信号通知
    void sig_press();
    void sig_release();
};

#endif // EYEICONBTN_H
