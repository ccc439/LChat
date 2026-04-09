#include "logindialog.h"
#include "ui_logindialog.h"
#include <QPainter>
#include <QPainterPath>
#include "httpmgr.h"
#include "tcpmgr.h"
#include "usermgr.h"

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    //设置qss errorMsg_label属性为 normal状态
    ui->errorMsg_label->setProperty("state","normal");
    //刷新qss
    repolish(ui->errorMsg_label);
    //http发送完成，响应结束时，进行处理
    connect(HttpMgr::getInstance().get(),&HttpMgr::sig_login_mod_finish,this,&LoginDialog::slot_login_mod_finish);
    //初始化_handlers
    initHttpHandlers();

    //更改忘记密码按钮的样式（隐藏按钮边框，鼠标悬浮变蓝色）
    ui->forget_btn->setStyleSheet(
        "QPushButton#forget_btn {"
        "    background-color: transparent;"
        "    border: none;"
        "    color: white;"
        "}"
        "QPushButton#forget_btn:hover {"
        "    color: blue;"
        "}"
        );
    initHead();
    //注册按钮触发 -> 发送信号给主界面，让主界面进行切换到注册界面
    connect(ui->reg_btn,&QPushButton::clicked,this,&LoginDialog::sig_SwitchReg);
    //“忘记密码”按钮触发 -> 发送信号给主界面，让主界面进行切换到重置密码界面
    connect(ui->forget_btn,&QPushButton::clicked,this,&LoginDialog::sig_SwitchReset);

    //连接tcp连接请求的信号和槽函数
    connect(this, &LoginDialog::sig_connect_tcp, TcpMgr::getInstance().get(), &TcpMgr::slot_tcp_connect);
    //连接tcp管理者发出的连接成功信号
    connect(TcpMgr::getInstance().get(), &TcpMgr::sig_con_success, this, &LoginDialog::slot_tcp_con_finish);
    //连接tcp管理者发出的登录失败信号
    connect(TcpMgr::getInstance().get(), &TcpMgr::sig_login_failed, this, &LoginDialog::slot_login_failed);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::showTip(QString str,bool isnormal){
    ui->errorMsg_label->setText(str);
    if(isnormal){
        ui->errorMsg_label->setProperty("state","normal");
    }
    else{
        ui->errorMsg_label->setProperty("state","error");
    }
    repolish(ui->errorMsg_label);
}

void LoginDialog::slot_login_mod_finish(ReqId req_id, QString res, ErrorCodes err)
{
    if(err != ErrorCodes::SUCCESS){
        //若失败，直接返回
        showTip(tr("出错了"),false);
        return;
    }

    //反序列化json
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    if(jsonDoc.isNull()){
        showTip(tr("json解析失败"),false);
        return;
    }
    if(!jsonDoc.isObject()){
        showTip(tr("json解析失败"),false);
        return;
    }

    //将json对象交给对应函数对象处理（通过req_id匹配对应函数对象）
    _handlers[req_id](jsonDoc.object());
    return;
}

void LoginDialog::initHttpHandlers()
{
    //注册获取登录回包逻辑
    _handlers.insert(ReqId::ID_LOGIN_USER, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("登录失败"),false);
            enableBtn(true);
            return;
        }
        auto email = jsonObj["email"].toString();

        //发送信号通知tcpMgr发送长链接
        ServerInfo si;
        si.Uid = jsonObj["uid"].toInt();
        si.Host = jsonObj["host"].toString();
        si.Port = jsonObj["port"].toString();
        si.Token = jsonObj["token"].toString();

        _uid = si.Uid;
        _token = si.Token;

        qDebug()<< "email is " << email << " uid is " << si.Uid <<" host is "
                 << si.Host << " Port is " << si.Port << " Token is " << si.Token;
        emit sig_connect_tcp(si);
    });
}

void LoginDialog::initHead(){
    // 加载图片
    QPixmap originalPixmap(":/res/login.png");
    // 设置图片自动缩放
    qDebug()<< originalPixmap.size() << ui->head_label->size();
    originalPixmap = originalPixmap.scaled(ui->head_label->size(),
                                           Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 创建一个和原始图片相同大小的QPixmap，用于绘制圆角图片
    QPixmap roundedPixmap(originalPixmap.size());
    roundedPixmap.fill(Qt::transparent); // 用透明色填充

    QPainter painter(&roundedPixmap);
    painter.setRenderHint(QPainter::Antialiasing); // 设置抗锯齿，使圆角更平滑
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 使用QPainterPath设置圆角
    QPainterPath path;
    path.addRoundedRect(0, 0, originalPixmap.width(), originalPixmap.height(), 10, 10); // 最后两个参数分别是x和y方向的圆角半径
    painter.setClipPath(path);

    // 将原始图片绘制到roundedPixmap上
    painter.drawPixmap(0, 0, originalPixmap);

    // 设置绘制好的圆角图片到QLabel上
    ui->head_label->setPixmap(roundedPixmap);

}

bool LoginDialog::checkPwdValid(){
    auto pwd = ui->pwd_edit->text();
    if(pwd.length() < 6 || pwd.length() > 15){
        qDebug() << "Pass length invalid";
        //提示长度不准确
        showTip(tr("密码长度应为6~15"),false);
        return false;
    }

    // 创建一个正则表达式对象，按照上述密码要求
    // 这个正则表达式解释：
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*.]{6,15}$");
    bool match = regExp.match(pwd).hasMatch();
    if(!match){
        //提示字符非法
        showTip(tr("不能包含非法字符且长度为(6~15)"),false);
        return false;;
    }

    return true;
}

bool LoginDialog::enableBtn(bool enabled)
{
    ui->login_btn->setEnabled(enabled);
    ui->reg_btn->setEnabled(enabled);
    return true;
}

void LoginDialog::on_login_btn_clicked()
{
    if(ui->email_edit->text() == ""){
        showTip(tr("邮箱不能为空"), false);
        return;
    }

    if(checkPwdValid() == false){
        return ;
    }

    enableBtn(false);
    auto email = ui->email_edit->text();
    auto pwd = ui->pwd_edit->text();
    //发送http请求登录
    QJsonObject json_obj;
    json_obj["email"] = email;
    json_obj["passwd"] = xorString(pwd);
    HttpMgr::getInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_login"),
                                        json_obj, ReqId::ID_LOGIN_USER,Modules::LOGIN_MODULE);
}

void LoginDialog::slot_tcp_con_finish(bool bsuccess)
{

    if(bsuccess){
        showTip(tr("聊天服务连接成功，正在登录..."),true);
        QJsonObject jsonObj;

        jsonObj["uid"] = _uid;
        jsonObj["token"] = _token;

        QJsonDocument doc(jsonObj);
        QByteArray jsonByteArray = doc.toJson(QJsonDocument::Compact);

        //发送tcp请求给chat server
        emit TcpMgr::getInstance()->sig_send_data(ReqId::ID_CHAT_LOGIN, jsonByteArray);

    }else{
        showTip(tr("网络异常"),false);
        enableBtn(true);
    }

}

void LoginDialog::slot_login_failed(int err)
{
    QString result = QString("登录失败, err is %1")
                         .arg(err);
    showTip(result,false);
    enableBtn(true);
}
