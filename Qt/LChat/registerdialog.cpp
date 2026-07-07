#include "registerdialog.h"
#include "ui_registerdialog.h"
#include "httpmgr.h"
#include <QLineEdit>

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RegisterDialog)
{
    ui->setupUi(this);

    //设置qss errorMsg_label属性为 normal状态
    ui->errorMsg_label->setProperty("state","normal");
    //刷新qss
    repolish(ui->errorMsg_label);
    //http发送完成，响应结束时，进行处理
    connect(HttpMgr::getInstance().get(),&HttpMgr::sig_reg_mod_finish,this,&RegisterDialog::slot_reg_mod_finish);
    //初始化_handlers
    initHttpHandlers();

    //设置悬停时的光标样式
    ui->pwd_visible->setCursor(Qt::PointingHandCursor);
    ui->confirm_visible->setCursor(Qt::PointingHandCursor);

    //connect小眼睛按钮与密码的交互
    connect(ui->pwd_visible,&EyeIconBtn::sig_press,[=](){
        ui->pwd_edit->setEchoMode(QLineEdit::EchoMode::Normal);
    });
    connect(ui->pwd_visible,&EyeIconBtn::sig_release,[=](){
        ui->pwd_edit->setEchoMode(QLineEdit::EchoMode::Password);
    });
    connect(ui->confirm_visible,&EyeIconBtn::sig_press,[=](){
        ui->confirm_edit->setEchoMode(QLineEdit::EchoMode::Normal);
    });
    connect(ui->confirm_visible,&EyeIconBtn::sig_release,[=](){
        ui->confirm_edit->setEchoMode(QLineEdit::EchoMode::Password);
    });

    //关于注册成功后跳转 提示界面的处理
    // 创建定时器
    reset_countdown();
    _countdown_timer = new QTimer(this);
    // 连接信号和槽
    connect(_countdown_timer, &QTimer::timeout, [this](){//每隔1秒接收一次timeout信号，执行一次lambda
        if(_countdown<=0){
            _countdown_timer->stop();
            emit sig_SwitchLogin();//倒计时归零时切换为登录界面（发送信号）
            reset_countdown();//重置_countdown为5
            return;
        }
        _countdown--;
        auto str = QString("注册成功，%1 s后返回登录界面").arg(_countdown);
        ui->tip_label1->setText(str);
    });

}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}

void RegisterDialog::on_get_code_btn_clicked()
{
    //获取用户在email_edit输入的邮箱
    auto email = ui->email_edit->text();
    //邮箱正则表达式
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    //检查用户输入的邮箱是否匹配正则表达式规则
    bool match = regex.match(email).hasMatch();

    if(match){
        //若匹配，发送http验证码
        QJsonObject json_obj;//json对象
        json_obj["email"] = email;//json存储匹配的email
        //发送http（url，json，模块功能id，模块）
        HttpMgr::getInstance()->PostHttpReq(
            QUrl(gate_url_prefix+"/get_verifycode"),json_obj,ReqId::ID_GET_VERIFY_CODE,Modules::REGISTER_MODULE);
    }
    else{
        //若未匹配，提示用户
        showTip(tr("邮箱地址有误"),false);//当应用程序切换语言时（如从英文切换到中文），tr() 会返回当前语言环境下对应的翻译文本。
    }
}

void RegisterDialog::slot_reg_mod_finish(ReqId req_id, QString res, ErrorCodes err)
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

//在注册界面上方errorMsg_label提示用户信息（ normal 或 error 状态）
void RegisterDialog::showTip(QString str,bool isnormal){
    ui->errorMsg_label->setText(str);
    if(isnormal){
        ui->errorMsg_label->setProperty("state","normal");
    }
    else{
        ui->errorMsg_label->setProperty("state","error");
    }
    repolish(ui->errorMsg_label);
}

void RegisterDialog::initHttpHandlers()
{
    //获取验证码
    _handlers.insert(ReqId::ID_GET_VERIFY_CODE,[this](const QJsonObject& jsonObj){
        int error = jsonObj["error"].toInt();//获取错误码
        if(error != ErrorCodes::SUCCESS){
            //若失败，直接返回
            showTip(tr("获取验证码失败"),false);
            return;
        }
        //若成功
        auto email = jsonObj["email"].toString();//获取邮箱
        showTip(tr("验证码已发送至邮箱，请注意查收"),true);
        qDebug()<<"email is "<<email;
    });

    //注册用户
    _handlers.insert(ReqId::ID_REG_USER, [this](const QJsonObject& jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("注册失败"),false);
            return;
        }

        showTip(tr("用户注册成功"), true);
        //跳转至注册成功后的 提示界面
        changeTipPage();

        auto uid = jsonObj["uid"].toInt();
        auto email = jsonObj["email"].toString();
        qDebug()<< "user uid is" << uid;
        qDebug()<< "email is " << email ;
    });
}

bool RegisterDialog::checkPwdValid(){
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

void RegisterDialog::on_confirm_btn_clicked()
{
    if(ui->user_edit->text() == ""){
        showTip(tr("用户名不能为空"), false);
        return;
    }
    if(ui->email_edit->text() == ""){
        showTip(tr("邮箱不能为空"), false);
        return;
    }
    if(ui->pwd_edit->text() == ""){
        showTip(tr("密码不能为空"), false);
        return;
    }

    if(checkPwdValid() == false){
        return ;
    }

    if(ui->confirm_edit->text() == ""){
        showTip(tr("确认密码不能为空"), false);
        return;
    }
    if(ui->confirm_edit->text() != ui->pwd_edit->text()){
        showTip(tr("密码和确认密码不匹配"), false);
        return;
    }
    if(ui->verificationCode_edit->text() == ""){
        showTip(tr("验证码不能为空"), false);
        return;
    }

    //发送http请求注册用户
    QJsonObject json_obj;
    json_obj["user"] = ui->user_edit->text();
    json_obj["email"] = ui->email_edit->text();
    json_obj["passwd"] = xorString(ui->pwd_edit->text());
    json_obj["confirm"] = xorString(ui->confirm_edit->text());
    json_obj["verifycode"] = ui->verificationCode_edit->text();

    json_obj["sex"] = 0;
    json_obj["icon"] = ":/res/head_1.jpg";
    json_obj["nick"] = ui->user_edit->text();
    HttpMgr::getInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_register"),
                                        json_obj, ReqId::ID_REG_USER,Modules::REGISTER_MODULE);
}

void RegisterDialog::changeTipPage(){
    _countdown_timer->stop();
    ui->stackedWidget->setCurrentWidget(ui->page_2);
    //启动定时器，发送timeout信号的 间隔为一秒
    _countdown_timer->start(1000);
}
void RegisterDialog::on_return_btn_clicked()
{
    _countdown_timer->stop();
    emit sig_SwitchLogin();
}
void RegisterDialog::setPage1(){
    ui->stackedWidget->setCurrentWidget(ui->page_1);
}
void RegisterDialog::reset_countdown(){
    _countdown = 5;
}

void RegisterDialog::on_cancel_btn_clicked()
{
    emit sig_SwitchLogin();
}

