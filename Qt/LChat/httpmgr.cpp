#include "httpmgr.h"

HttpMgr::HttpMgr()
{
    connect(this,&HttpMgr::sig_http_finish,this,&HttpMgr::slot_http_finish);//connect sig_http_finish -> slot_http_finish
}

HttpMgr::~HttpMgr(){}

void HttpMgr::PostHttpReq(QUrl url,QJsonObject json,ReqId req_id,Modules mod)
{
    QByteArray data = QJsonDocument(json).toJson();//Qt JSON序列化
    QNetworkRequest request(url);//创建网络请求对象并绑定目标URL
    //设置请求头类型为JSON，告知服务器接收JSON格式数据
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    //设置请求体长度
    request.setHeader(QNetworkRequest::ContentLengthHeader,QByteArray::number(data.length()));
    auto self = shared_from_this();//捕获当前类的智能指针，避免在lambda表达式中出现悬空指针问题
    //通过QNetworkAccessManager发送POST请求
    QNetworkReply * reply = _manager.post(request,data);
    //connect绑定 请求完成时处理响应（lambda）
    connect(reply,&QNetworkReply::finished,[self,reply,req_id,mod](){
        //处理错误情况
        if(reply->error() != QNetworkReply::NoError){
            qDebug()<<reply->errorString();
            //发送信号通知：http发送完成，响应结束
            emit self->sig_http_finish(req_id,"",ErrorCodes::ERR_NETWORK,mod);
            reply->deleteLater();//回收reply
            return;
        }
        //无错误
        QString res = reply->readAll();
        //发送信号通知：http发送完成，响应结束
        emit self->sig_http_finish(req_id,res,ErrorCodes::SUCCESS,mod);
        reply->deleteLater();//回收reply
    });
}

void HttpMgr::slot_http_finish(ReqId req_id, QString res, ErrorCodes err, Modules mod)
{
    //注册模块
    if(mod == Modules::REGISTER_MODULE){
        //发送信号通知注册模块：http发送完成，响应结束
        emit sig_reg_mod_finish(req_id,res,err);
    }
    //重置密码模块
    if(mod == Modules::RESET_MODULE){
        //发送信号通知重置密码模块：http发送完成，响应结束
        emit sig_reset_mod_finish(req_id,res,err);
    }
    //登录模块
    if(mod == Modules::LOGIN_MODULE){
        //发送信号通知登录模块：http发送完成，响应结束
        emit sig_login_mod_finish(req_id,res,err);
    }
}











