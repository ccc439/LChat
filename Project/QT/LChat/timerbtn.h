#ifndef TIMERBTN_H
#define TIMERBTN_H
#include <QPushButton>
#include <QTimer>

class TimerBtn : public QPushButton
{
public:
    TimerBtn(QWidget *parent = nullptr);
    ~ TimerBtn();

    // 重写mouseReleaseEvent，鼠标左键释放时触发
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
private:
    QTimer  *_timer;
    int _counter;
};

#endif // TIMERBTN_H
