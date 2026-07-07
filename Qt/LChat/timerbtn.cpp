#include "timerbtn.h"
#include <QMouseEvent>
#include <QDebug>

TimerBtn::TimerBtn(QWidget *parent):QPushButton(parent),_counter(10)
{
    _timer = new QTimer(this);

    connect(_timer, &QTimer::timeout, [this](){
        _counter--;
        if(_counter <= 0){
            _timer->stop();
            _counter = 10;
            this->setText("获取");
            this->setEnabled(true);
            return;
        }
        this->setText(QString::number(_counter));
    });
}

TimerBtn::~TimerBtn()
{
    _timer->stop();
}

void TimerBtn::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        // 在这里处理鼠标左键释放事件
        qDebug() << "MyButton was released!";
        this->setEnabled(false);//将按钮设置为禁用状态
        this->setText(QString::number(_counter));//显示倒计时
        _timer->start(1000);//每秒触发一次定时器的timeout()信号
        emit clicked();//发送信号通知其他界面
    }
    // 调用基类的mouseReleaseEvent以保持按钮的默认行为
    QPushButton::mouseReleaseEvent(e);
}
