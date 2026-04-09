#ifndef RESETDIALOG_H
#define RESETDIALOG_H

#include <QDialog>
#include "global.h"

namespace Ui {
class ResetDialog;
}

class ResetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ResetDialog(QWidget *parent = nullptr);
    ~ResetDialog();
private slots:
    void on_get_code_btn_clicked();//当按下get_code（获取邮箱）按钮时调用，若用户输入邮箱符合要求，向服务器发送http请求
    void slot_reset_mod_finish(ReqId req_id,QString res,ErrorCodes err);//http发送完成，响应结束时，进行处理
    void on_confirm_btn_clicked();//当按下confirm_btn按钮时调用，执行注册功能
    void on_cancel_btn_clicked();//当按下“取消”按钮时调用，返回登录界面

private:
    Ui::ResetDialog *ui;
    //在注册界面上方errorMsg_label提示用户信息（ normal 或 error 状态）
    void showTip(QString str,bool isnormal);
    //初始化_handlers
    void initHttpHandlers();
    //检验密码是否符合规范
    bool checkPwdValid();

    //根据ReqId，取出对应函数对象
    QMap<ReqId,std::function<void(const QJsonObject&)>> _handlers;
signals:
    void sig_SwitchLogin();//切换到登录界面
};

#endif // RESETDIALOG_H
