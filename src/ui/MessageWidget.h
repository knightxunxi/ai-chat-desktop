#pragma once

#include "core/MessageRole.h"

#include <QFrame>

class QLabel;
class QPushButton;
class QVBoxLayout;

// 学习注释：单条消息气泡组件，负责角色样式、普通文本展示、代码块展示和复制操作。
// 使用模块：ChatView 创建它，MessageWidgetTest 覆盖复制和代码块行为。
class MessageWidget : public QFrame
{
    Q_OBJECT

public:
    explicit MessageWidget(MessageRole role, const QString &content, QWidget *parent = nullptr);

    // 功能：返回消息角色；使用模块：测试和未来消息操作可用。
    MessageRole role() const;
    // 功能：返回当前消息原文；使用模块：复制和测试。
    QString content() const;
    // 功能：更新消息内容并重建显示；使用模块：流式助手回复。
    void setContent(const QString &content);

private:
    // 功能：复制整条消息；使用模块：消息右上角复制按钮。
    void copyContentToClipboard() const;
    // 功能：复制指定文本；使用模块：整条消息复制和代码块复制共用。
    void copyTextToClipboard(const QString &text) const;
    // 功能：解析并重建普通文本/代码块控件；使用模块：构造和 setContent。
    void rebuildContent();
    // 功能：清理旧内容控件；使用模块：rebuildContent。
    void clearContentWidgets();
    // 功能：添加普通文本片段；使用模块：Markdown 代码块拆分后的文本部分。
    void addTextSegment(const QString &text);
    // 功能：添加代码块和“Copy code”按钮；使用模块：Markdown fenced code block 展示。
    void addCodeBlock(const QString &language, const QString &code);

    MessageRole m_role = MessageRole::User;  // 功能：消息角色；使用模块：样式和角色标签。
    QString m_content;                       // 功能：消息原始内容；使用模块：重建显示和复制。
    QLabel *m_roleLabel = nullptr;           // 功能：显示 User/Assistant；使用模块：消息头部。
    QVBoxLayout *m_contentLayout = nullptr;  // 功能：承载文本段和代码块；使用模块：rebuildContent。
    QPushButton *m_copyButton = nullptr;     // 功能：复制整条消息按钮；使用模块：消息头部操作。
};
