#pragma once

#include "core/ChatMessage.h"

#include <QDateTime>
#include <QString>
#include <QVector>
#include <QUuid>

// 学习注释：聊天会话模型，包含标题、角色提示词和消息列表。
// 使用模块：ApplicationController 管理当前会话，ChatHistoryStorage 负责保存和读取，MainWindow 展示会话列表。
struct ChatSession {
    QString id;                         // 功能：会话唯一标识；使用模块：数据库 sessions 表主键和 UI 列表项 UserRole。
    QString title;                      // 功能：会话标题；使用模块：侧边栏会话列表、重命名、导出默认文件名。
    QString systemPrompt;               // 功能：角色提示词；使用模块：RolePromptDialog 编辑，请求体中作为 system 消息发送。
    QDateTime createdAt;                // 功能：会话创建时间 UTC；使用模块：数据库记录和导出。
    QDateTime updatedAt;                // 功能：最后更新时间 UTC；使用模块：会话列表按最近更新排序。
    bool isFavorite = false;            // 功能：是否收藏；使用模块：会话列表筛选和收藏按钮。
    bool isArchived = false;            // 功能：是否归档；使用模块：默认列表隐藏归档会话，归档筛选显示。
    QVector<ChatMessage> messages;      // 功能：当前会话消息集合；使用模块：ChatView 展示、AI 请求上下文、历史存储。

    // 功能：创建一个空白新会话；使用模块：应用首次启动、点击新建会话、删除后补空会话。
    static ChatSession createDefault()
    {
        ChatSession session;
        session.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        session.title = QStringLiteral("New Chat");
        session.createdAt = QDateTime::currentDateTimeUtc();
        session.updatedAt = session.createdAt;
        return session;
    }

    // 功能：判断当前会话是否使用了角色提示词；使用模块：保存会话前判断是否值得持久化。
    bool hasSystemPrompt() const
    {
        return !systemPrompt.trimmed().isEmpty();
    }

    // 功能：追加消息并刷新会话更新时间；使用模块：ApplicationController 处理用户输入和助手回复。
    ChatMessage addMessage(MessageRole role, const QString &content)
    {
        ChatMessage message = ChatMessage::create(id, role, content);
        messages.append(message);
        updatedAt = message.createdAt;
        return message;
    }
};
