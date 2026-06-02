#pragma once

#include "core/AppConfig.h"
#include "core/ChatSession.h"
#include "services/RequestErrorCategory.h"

#include <QObject>

// 学习注释：AI 服务访问抽象接口，隐藏具体服务商 HTTP 实现。
// 使用模块：ApplicationController 只依赖该接口语义，OpenAICompatibleClient 提供实际实现。
class AIClient : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~AIClient() override = default;

    // 功能：根据配置和会话上下文发起聊天请求；使用模块：ApplicationController::startAssistantRequest。
    virtual void sendChat(const AppConfig &config, const ChatSession &session) = 0;
    // 功能：取消当前请求；使用模块：停止生成、窗口关闭前清理。
    virtual void cancel() = 0;

signals:
    void textDeltaReceived(const QString &delta); // 功能：流式文本增量；使用模块：ApplicationController 拼接助手回复。
    void requestFinished(); // 功能：请求正常完成；使用模块：ApplicationController 保存最终会话。
    void requestFailed(const QString &message, RequestErrorCategory category); // 功能：请求失败通知；使用模块：错误展示和重试状态。
};
