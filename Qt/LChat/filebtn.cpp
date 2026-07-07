#include "filebtn.h"

#include <QVariant>

FileBtn::FileBtn(QWidget *parent):QPushButton (parent)
{
    setCursor(Qt::PointingHandCursor); // 设置光标为小手
    setStyleSheet("QPushButton {"
                  "    border-image: url(:/res/filedir_normal.png);"
                  "}"
                  "QPushButton:hover {"
                  "    border-image: url(:/res/filedir_hover.png);"
                  "}"
                  );
}
