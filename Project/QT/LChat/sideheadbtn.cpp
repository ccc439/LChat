#include "sideheadbtn.h"

#include <QVariant>

SideHeadBtn::SideHeadBtn(QWidget *parent):QPushButton (parent)
{
    setCursor(Qt::PointingHandCursor); // 设置光标为小手
    setStyleSheet("QPushButton {"
                  "    border-image: url(:/res/head_1.jpg);"
                  "}"
                  );
}
