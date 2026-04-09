#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include "global.h"
#include "userdata.h"
#include <QListWidgetItem>

namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();
    //测试
    void addChatUserList();
private:
    Ui::ChatDialog *ui;
    void ShowChatUIMode(ChatUIMode mode);//要展示的界面模式（ChatMode聊天模式 ContactMode通讯录模式 SearchMode搜索列表模式）
    ChatUIMode _mode;//当前聊天界面的模式
    ChatUIMode _premode;//先前聊天界面的模式

    bool _b_loading;

    //QList<StateWidget*> _lb_list;
    QMap<int, QListWidgetItem*> _chat_items_added;

    int _cur_chat_uid;

    void SetSelectChatPage(int uid = 0);
    void loadMoreChatUser();
    void loadMoreConUser();
    void SetSelectChatItem(int uid = 0);
    QWidget* _last_widget;

    void UpdateChatMsg(std::vector<std::shared_ptr<TextChatData> > msgdata);

    //使得鼠标点击搜索栏外的区域时，关闭搜索栏
    bool eventFilter(QObject *watched, QEvent *event);
    void handleGlobalMousePress(QMouseEvent *event);
private slots:
    void slot_loading_chat_user();//滚动到底部时，加载新的联系人
    void slot_apply_friend(std::shared_ptr<AddFriendApply> apply);
    void slot_add_auth_friend(std::shared_ptr<AuthInfo> auth_info);
    void slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp);
    void slot_jump_chat_item(std::shared_ptr<SearchInfo> si);
    void slot_loading_contact_user();
    void slot_friend_info_page(std::shared_ptr<UserInfo> user_info);
    void slot_switch_apply_friend_page();
    void slot_jump_chat_item_from_infopage(std::shared_ptr<UserInfo> user_info);
    void slot_item_clicked(QListWidgetItem *item);
    void slot_append_send_chat_msg(std::shared_ptr<TextChatData> msgdata);
    void slot_text_chat_msg(std::shared_ptr<TextChatMsg> msg);
signals:
    //通知另一个按钮ChatMode被选中
    void sig_jump_chat();
};

#endif // CHATDIALOG_H
