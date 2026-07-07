#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "logindialog.h"
#include "registerdialog.h"
#include "resetdialog.h"
#include "chatdialog.h"
#include <QStackedWidget>
/*****************************************************
*
*   @file       mainwindow.h
*   @author     L
*   @date       2026.1.24
*
*****************************************************/


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    LoginDialog * _login_dlg;//鐧诲綍鐣岄潰
    RegisterDialog * _register_dlg;//娉ㄥ唽鐣岄潰
    ResetDialog * _reset_dlg;//閲嶇疆瀵嗙爜鐣岄潰
    ChatDialog * _chat_dlg;//鑱婂ぉ鐣岄潰
    QStackedWidget *_stack;//鐢ㄥ爢鏍堢獥鍙ｇ鐞哶login_dlg鍜宊register_dlg...

public slots:
};
#endif // MAINWINDOW_H
