#include "clickedbtn.h"
#include <QVariant>

ClickedBtn::ClickedBtn(QWidget *parent):QPushButton (parent)
{
    setCursor(Qt::PointingHandCursor); // 设置光标为小手
    setStyleSheet("QPushButton {"
                  "    border-image: url(:/res/add_friend_normal.png);"
                  "}"
                  "QPushButton:hover {"
                  "    border-image: url(:/res/add_friend_hover.png);"
                  "}"
                  );
}

