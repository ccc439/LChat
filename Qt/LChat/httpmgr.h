#ifndef HTTPMGR_H
#define HTTPMGR_H
#include "singleton.h"
#include <QString>
#include <QUrl>
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonDocument>

//CRTP
class HttpMgr:public QObject,public Singleton<HttpMgr>,public std::enable_shared_from_this<HttpMgr>
{
    Q_OBJECT
public:
    void PostHttpReq(QUrl url,QJsonObject json,ReqId req_id,Modules mod);//发送http（url，json，模块功能id，模块）
private:
    HttpMgr();
    ~HttpMgr();
    QNetworkAccessManager _manager;//http管理者
private slots:
    void slot_http_finish(ReqId req_id,QString res,ErrorCodes err,Modules mod);//sig_http_finish的槽函数
signals:
    void sig_http_finish(ReqId req_id,QString res,ErrorCodes err,Modules mod);//http发送完成，响应结束的信号（模块功能id，返回的内容，错误码，模块）
    void sig_reg_mod_finish(ReqId req_id,QString res,ErrorCodes err);//通知注册模块：http发送完成，响应结束的信号（模块功能id，返回的内容，错误码）
    void sig_reset_mod_finish(ReqId req_id,QString res,ErrorCodes err);//通知重置密码模块：http发送完成，响应结束的信号（模块功能id，返回的内容，错误码）
    void sig_login_mod_finish(ReqId req_id,QString res,ErrorCodes err);//通知登录模块：http发送完成，响应结束的信号

friend class Singleton<HttpMgr>;//Singleton单例模板中，静态变量static T _instance会尝试调用HttpMgr的构造函数（T 是 HttpMgr）
};

#endif // HTTPMGR_H
