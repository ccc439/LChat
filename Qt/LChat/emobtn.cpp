#include "emobtn.h"
#include <QVariant>

EmoBtn::EmoBtn(QWidget *parent):QPushButton (parent)
{
    setCursor(Qt::PointingHandCursor); // 设置光标为小手
    setStyleSheet("QPushButton {"
                  "    border-image: url(:/res/smile_normal.png);"
                  "}"
                  "QPushButton:hover {"
                  "    border-image: url(:/res/smile_hover.png);"
                  "}"
                  );
}
