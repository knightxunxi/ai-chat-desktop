#pragma once

#include "core/AppConfig.h"
#include "core/ChatMessage.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QVector>

// 功能：调用 AI API 将裁剪后的历史对话总结为一句话摘要。
// 使用模块：ContextWindowManager 在压缩上下文时生成对话摘要。
class SummaryAPIClient : public QObject {
    Q_OBJECT

public:
    // 功能：使用父对象构造，后续通过 reconfigure() 设置配置。
    explicit SummaryAPIClient(QObject *parent = nullptr);
    // 功能：使用 AppConfig 构造并直接设置配置。
    explicit SummaryAPIClient(const AppConfig &config, QObject *parent = nullptr);

    // 功能：更新内部配置引用；使用模块：ApplicationController::initialize() 加载配置后调用。
    void reconfigure(const AppConfig &config);
    // 功能：将裁剪后的消息列表生成为摘要文本；使用模块：ContextWindowManager::processMessages。
    QString generateSummary(const QVector<ChatMessage> &trimmedMessages);
    // 功能：设置 HTTP 请求超时时间（毫秒）。
    void setTimeout(int msecs);
    // 功能：构建摘要提示词；使用模块：generateSummary 第一步。
    static QString buildSummaryPrompt(const QVector<ChatMessage> &trimmedMessages);

private:
    // 功能：构建 JSON 请求体；使用模块：generateSummary 第二步。
    QByteArray buildSummaryRequestBody(const QString &prompt) const;
    // 功能：从 API 响应中提取摘要文本；使用模块：generateSummary 最后一步。
    QString extractSummaryFromResponse(const QByteArray &responseBody) const;

    AppConfig m_config;
    QNetworkAccessManager m_networkManager;
    int m_timeoutMs = 5000;
};
