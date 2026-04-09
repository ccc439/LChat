#ifndef CLICKEDBTN_H
#define CLICKEDBTN_H
#include <QPushButton>

class ClickedBtn:public QPushButton
{
    Q_OBJECT
public:
    explicit ClickedBtn(QWidget *parent = nullptr);
signals:
};

#endif // CLICKEDBTN_H
