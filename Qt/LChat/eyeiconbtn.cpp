#include "eyeiconbtn.h"

EyeIconBtn::EyeIconBtn(QWidget *parent):QPushButton(parent){
    //设置小眼睛样式
    setStyleSheet("QPushButton {"
                  "    border-image: url(:/res/unvisible.png);"
                  "}"
                  "QPushButton:hover {"
                  "    border-image: url(:/res/unvisible_hover.png);"
                  "}"
                  "QPushButton:pressed {"
                  "    border-image: url(:/res/visible_hover.png);"
                  "}"
                  );
}

void EyeIconBtn::mousePressEvent(QMouseEvent *e){
    QPushButton::mousePressEvent(e);//保持默认行为
    emit sig_press();
}

void EyeIconBtn::mouseReleaseEvent(QMouseEvent *e){
    QPushButton::mouseReleaseEvent(e);//保持默认行为
    emit sig_release();
}
