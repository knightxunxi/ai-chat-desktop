#pragma once

#include "core/MessageRole.h"

#include <QDateTime>
#include <QString>
#include <QUuid>

// 学习注释：单条聊天消息模型，表示用户或助手在某个会话中的一条内容。
// 使用模块：ChatSession 聚合消息，ChatHistoryStorage 持久化消息，ChatView/MessageWidget 展示消息。
struct ChatMessage {
    QString id;                         // 功能：消息唯一标识；使用模块：数据库 messages 表主键。
    QString sessionId;                  // 功能：所属会话 ID；使用模块：ChatHistoryStorage 关联 sessions 和 messages。
    MessageRole role = MessageRole::User; // 功能：区分用户/助手消息；使用模块：MessageWidget 决定样式和角色标签。
    QString content;                    // 功能：消息正文；使用模块：UI 展示、Markdown 导出、历史记录保存。
    QDateTime createdAt;                // 功能：创建时间 UTC；使用模块：数据库排序和导出时间显示。

    // 功能：创建带 UUID 和 UTC 时间的新消息；使用模块：ChatSession::addMessage。
    static ChatMessage create(const QString &sessionId, MessageRole role, const QString &content)
    {
        ChatMessage message;
        message.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        message.sessionId = sessionId;
        message.role = role;
        message.content = content;
        message.createdAt = QDateTime::currentDateTimeUtc();
        return message;
    }
};
