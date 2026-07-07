#ifndef FILEBTN_H
#define FILEBTN_H
#include <QPushButton>

class FileBtn:public QPushButton
{
    Q_OBJECT
public:
    explicit FileBtn(QWidget *parent = nullptr);
signals:
};

#endif // FILEBTN_H
