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
    LoginDialog * _login_dlg;//登录界面
    RegisterDialog * _register_dlg;//注册界面
    ResetDialog * _reset_dlg;//重置密码界面
    ChatDialog * _chat_dlg;//聊天界面
    QStackedWidget *_stack;//用堆栈窗口管理_login_dlg和_register_dlg...

public slots:
};
#endif // MAINWINDOW_H
