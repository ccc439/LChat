#ifndef TCPMGR_H
#define TCPMGR_H
#include <QTcpSocket>
#include "global.h"
#include "singleton.h"
#include "userdata.h"

//tcp管理类
class TcpMgr:public QObject, public Singleton<TcpMgr>,
               public std::enable_shared_from_this<TcpMgr>
{
    Q_OBJECT
public:
    TcpMgr();
private:
    QTcpSocket _socket;
    QString _host;
    uint16_t _port;
    QByteArray _buffer;//发送数据的消息体
    bool _b_recv_pending;//是否正在等待接收一个不完整的消息体（_buffer）剩余部分
    quint16 _message_id;//发送数据的id
    quint16 _message_len;//发送数据消息体（_buffer）的长度
    //
    void initHandlers();
    void handleMsg(ReqId id, int len, QByteArray data);
    QMap<ReqId, std::function<void(ReqId id, int len, QByteArray data)>> _handlers;
public slots:
    void slot_tcp_connect(ServerInfo);//tcp连接成功时调用，
    void slot_send_data(ReqId reqId, QByteArray dataBytes);//数据发送成功时调用，
signals:
    void sig_con_success(bool bsuccess);//tcp连接成功时发送
    void sig_send_data(ReqId reqId, QByteArray data);//数据发送成功时发送
    void sig_SwichChat();//切换到聊天界面
    void sig_login_failed(int);//
    void sig_user_search(std::shared_ptr<SearchInfo>);//
    void sig_friend_apply(std::shared_ptr<AddFriendApply>);//
    void sig_add_auth_friend(std::shared_ptr<AuthInfo>);//
    void sig_auth_rsp(std::shared_ptr<AuthRsp>);//
    void sig_text_chat_msg(std::shared_ptr<TextChatMsg> msg);
friend class Singleton<TcpMgr>;
};

#endif // TCPMGR_H
