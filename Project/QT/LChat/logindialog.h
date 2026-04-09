#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "global.h"

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    void initHead();//设置默认头像框样式
    ~LoginDialog();

private:
    Ui::LoginDialog *ui;
    //在登录界面上方errorMsg_label提示用户信息（ normal 或 error 状态）
    void showTip(QString str,bool isnormal);
    //检验密码是否符合规范
    bool checkPwdValid();
    bool enableBtn(bool enabled);
    //初始化_handlers
    void initHttpHandlers();

    //根据ReqId，取出对应函数对象
    QMap<ReqId,std::function<void(const QJsonObject&)>> _handlers;

    int _uid;
    QString _token;
signals:
    //切换为注册界面（给主界面发送的信号，让主界面进行切换）
    void sig_SwitchReg();
    //切换为重置密码界面（给主界面发送的信号，让主界面进行切换）
    void sig_SwitchReset();
    //
    void sig_connect_tcp(ServerInfo);
private slots:
    void on_login_btn_clicked();//按下“登录”按钮后调用
    void slot_login_mod_finish(ReqId req_id,QString res,ErrorCodes err);//http发送完成，响应结束时，进行处理
    void slot_tcp_con_finish(bool bsuccess);//tcp连接完成，
    void slot_login_failed(int err);//登录失败
};

#endif // LOGINDIALOG_H
