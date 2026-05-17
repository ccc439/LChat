#include "clickedoncelabel.h"

ClickedOnceLabel::ClickedOnceLabel(QWidget *parent):QLabel(parent)
{
    setCursor(Qt::PointingHandCursor);
}


void ClickedOnceLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(this->text());
        return;
    }
    // 璋冪敤鍩虹被鐨刴ousePressEvent浠ヤ繚璇佹甯哥殑浜嬩欢澶勭悊
    QLabel::mousePressEvent(event);
}

