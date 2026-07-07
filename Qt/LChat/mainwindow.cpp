#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tcpmgr.h"
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 创建堆栈窗口
    _stack = new QStackedWidget(this);
    setCentralWidget(_stack);

    // 一次性创建所有界面
    _login_dlg = new LoginDialog();
    _register_dlg = new RegisterDialog();
    _reset_dlg = new ResetDialog();
    _chat_dlg = new ChatDialog();

    // 添加到堆栈
    _stack->addWidget(_login_dlg);
    _stack->addWidget(_register_dlg);
    _stack->addWidget(_reset_dlg);
    _stack->addWidget(_chat_dlg);

    // 显示登录界面
    _stack->setCurrentWidget(_login_dlg);

    // 连接信号
    connect(_login_dlg, &LoginDialog::sig_SwitchReg, [this]() {//从登录界面切换到注册界面
        _register_dlg->setPage1();
        _stack->setCurrentWidget(_register_dlg);
    });

    connect(_register_dlg, &RegisterDialog::sig_SwitchLogin, [this]() {//从注册界面切换到登录界面
        _stack->setCurrentWidget(_login_dlg);
    });

    connect(_login_dlg, &LoginDialog::sig_SwitchReset, [this]() {//从登录界面切换到重置密码界面
        _stack->setCurrentWidget(_reset_dlg);
    });

    connect(_reset_dlg, &ResetDialog::sig_SwitchLogin, [this]() {//重置密码界面切换到登录界面
        _stack->setCurrentWidget(_login_dlg);
    });

    connect(TcpMgr::getInstance().get(), &TcpMgr::sig_SwichChat, [this]() {//从登录界面切换到聊天界面（信号是TcpMgr发出的）
        _stack->setCurrentWidget(_chat_dlg);
    });

    //变化窗口大小
    connect(_stack, &QStackedWidget::currentChanged, [this]() {
        QWidget* currentPage = _stack->currentWidget();

        // 强制布局计算
        currentPage->adjustSize();

        // 使用当前页面的大小
        QSize newSize = currentPage->size();

        // 如果页面大小为0，尝试使用sizeHint
        if (newSize.isEmpty()) {
            newSize = currentPage->sizeHint();
        }

        this->resize(newSize);

        QTimer::singleShot(0, this, [this]() {
            this->setFixedSize(this->size());
        });
    });
    //测试
    // QTimer::singleShot(2000, []() {
    //     emit TcpMgr::getInstance()->sig_SwichChat();
    // });
}

MainWindow::~MainWindow()
{
    delete ui;
}

