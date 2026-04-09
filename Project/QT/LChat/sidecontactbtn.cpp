#include "sidecontactbtn.h"
#include <QVariant>

SideContactBtn::SideContactBtn(QWidget *parent):QPushButton (parent)
{
    setCursor(Qt::PointingHandCursor); // 设置光标为小手
    // 设置为可选中按钮
    setCheckable(true);
    setChecked(false);//默认为未选中

    setStyleSheet("QPushButton {"
                  "    border-image: url(:/res/contact_normal.png);"
                  "}"
                  "QPushButton:hover {"
                  "    border-image: url(:/res/contact_hover.png);"
                  "}"
                  "QPushButton:pressed {"
                  "    border-image: url(:/res/contact_press.png);"
                  "}"
                  "QPushButton:checked {"
                  "    border-image: url(:/res/contact_press.png);"  // 选中状态保持press图片
                  "}"
                  );

    //连接clicked信号，第一次点击后锁定
    connect(this, &SideContactBtn::clicked, this, [this](){
        if(!isChecked()) {
            setChecked(true);  // 第一次点击：设置为选中状态
        }
        // 第二次点击：什么都不做，保持选中状态
    });
}

void SideContactBtn::mousePressEvent(QMouseEvent *e){
    QPushButton::mousePressEvent(e);//保持默认行为
    emit sig_press_contact();
}

void SideContactBtn::nextCheckState(){

}

void SideContactBtn::slot_press_chat(){
    setChecked(false);  // 取消选中，恢复normal状态
}
