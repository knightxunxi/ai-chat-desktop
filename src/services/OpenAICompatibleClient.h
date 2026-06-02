#pragma once

#include "services/AIClient.h"
#include "services/StreamParser.h"

#include <QNetworkAccessManager>
#include <QPointer>
#include <QUrl>

class QNetworkReply;

// 学习注释：OpenAI-compatible HTTP 客户端，负责把本地会话转换为网络请求并解析流式响应。
// 使用模块：ApplicationController 持有该对象，DeepSeek 等兼容 OpenAI 接口的服务商都走这里。
class OpenAICompatibleClient : public AIClient
{
    Q_OBJECT

public:
    explicit OpenAICompatibleClient(QObject *parent = nullptr);

    // 功能：生成 chat/completions 请求 JSON；使用模块：sendChat 和 OpenAICompatibleClientTest。
    static QByteArray buildRequestBody(const AppConfig &config, const ChatSession &session);
    // 功能：把 HTTP 状态码归类为认证、额度、模型、服务端等错误；使用模块：handleFinished 和单元测试。
    static RequestErrorCategory classifyHttpStatus(int statusCode);

    // 功能：发起一次聊天请求；使用模块：ApplicationController::startAssistantRequest。
    void sendChat(const AppConfig &config, const ChatSession &session) override;
    // 功能：取消当前网络请求；使用模块：停止生成和关闭窗口时调用。
    void cancel() override;

private:
    // 功能：把 baseUrl 转换成 /chat/completions 地址；使用模块：sendChat。
    QUrl chatCompletionsUrl(const QString &baseUrl) const;
    // 功能：从错误响应体中提取可读错误；使用模块：handleFinished。
    QString extractErrorMessage(const QByteArray &body, const QString &fallback) const;
    // 功能：处理流式响应中的新增数据；使用模块：QNetworkReply::readyRead 信号。
    void handleReadyRead();
    // 功能：处理请求完成、错误归类和资源释放；使用模块：QNetworkReply::finished 信号。
    void handleFinished();

    QNetworkAccessManager m_networkManager; // 功能：Qt 网络管理器；使用模块：sendChat 创建 HTTP 请求。
    QPointer<QNetworkReply> m_currentReply; // 功能：当前请求对象的安全指针；使用模块：cancel/handleReadyRead/handleFinished。
    StreamParser m_streamParser;            // 功能：解析 SSE 文本流；使用模块：handleReadyRead。
    QByteArray m_errorBody;                 // 功能：缓存非 2xx 响应体；使用模块：handleFinished 提取错误信息。
    bool m_doneReceived = false;            // 功能：记录是否收到 [DONE]；使用模块：区分完整响应和中断响应。
};
