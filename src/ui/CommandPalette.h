#pragma once

#include <QFrame>
#include <QStringList>

class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QVBoxLayout;

// V19: 命令面板 — 类似 VS Code Command Palette 的模糊搜索弹出框
// 使用模块：MainWindow 持有，Ctrl+Shift+P 打开
class CommandPalette : public QFrame
{
    Q_OBJECT

public:
    struct Command {
        QString id;        // 命令标识
        QString label;     // 显示文本（中/英文）
        QString shortcut;  // 快捷键提示（可选）
        QVariant data;     // 携带数据（如 sessionId）
    };

    explicit CommandPalette(QWidget *parent = nullptr);

    void setCommands(const QVector<Command> &commands);
    void showAtCenter();
    void hidePalette();

signals:
    void commandSelected(const QString &commandId, const QVariant &data);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void filterItems(const QString &text);

    QLineEdit *m_searchInput = nullptr;
    QListWidget *m_resultList = nullptr;
    QVector<Command> m_commands;
};
