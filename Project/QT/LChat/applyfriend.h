#ifndef APPLYFRIEND_H
#define APPLYFRIEND_H

#include <QDialog>
#include "clickedlabel.h"
#include "friendlabel.h"
#include "userdata.h"
namespace Ui {
class ApplyFriend;
}

class ApplyFriend : public QDialog
{
    Q_OBJECT

public:
    explicit ApplyFriend(QWidget *parent = nullptr);
    ~ApplyFriend();
    void InitTipLbs();
    void AddTipLbs(ClickedLabel*, QPoint cur_point, QPoint &next_point, int text_width, int text_height);
    bool eventFilter(QObject *obj, QEvent *event);
    void SetSearchInfo(std::shared_ptr<SearchInfo> si);
private:
    void resetLabels();
    Ui::ApplyFriend *ui;
    //
    QMap<QString, ClickedLabel*> _add_labels;
    std::vector<QString> _add_label_keys;
    QPoint _label_point;
    //
    QMap<QString, FriendLabel*> _friend_labels;
    std::vector<QString> _friend_label_keys;
    void addLabel(QString name);
    std::vector<QString> _tip_data;
    QPoint _tip_cur_point;
    std::shared_ptr<SearchInfo> _si;
public slots:
    //
    void ShowMoreLabel();
    //
    void SlotLabelEnter();
    //
    void SlotRemoveFriendLabel(QString);
    //
    void SlotChangeFriendLabelByTip(QString, ClickLbState);
    //
    void SlotLabelTextChange(const QString& text);
    //
    void SlotLabelEditFinished();
    //
    void SlotAddFirendLabelByClickTip(QString text);
    //
    void SlotApplySure();
    //
    void SlotApplyCancel();
};

#endif // APPLYFRIEND_H
