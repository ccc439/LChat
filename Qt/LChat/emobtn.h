#ifndef EMOBTN_H
#define EMOBTN_H

#include <QPushButton>

class EmoBtn:public QPushButton
{
    Q_OBJECT
public:
    explicit EmoBtn(QWidget *parent = nullptr);
signals:
};
#endif // EMOBTN_H
