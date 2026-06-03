#include "app/SummaryAPIClient.h"

#include "support/AppLogger.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

// --- 默认构造函数：使用默认 AppConfig，供 ApplicationController 成员初始化 ---

SummaryAPIClient::SummaryAPIClient(QObject *parent)
    : QObject(parent)
    , m_config(AppConfig::defaultConfig())
    , m_networkManager(this)
{
}

// --- 带配置构造函数 ---

SummaryAPIClient::SummaryAPIClient(const AppConfig &config, QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_networkManager(this)
{
}

void SummaryAPIClient::reconfigure(const AppConfig &config)
{
    m_config = config;
}

void SummaryAPIClient::setTimeout(int msecs)
{
    m_timeoutMs = msecs;
}

// 功能：将裁剪消息列表构建为摘要提示词。
// 格式："请将以下对话历史总结为一句话摘要（50-80字）：\n\n[逐条消息]"
QString SummaryAPIClient::buildSummaryPrompt(const QVector<ChatMessage> &trimmedMessages)
{
    QString prompt = QStringLiteral("请将以下对话历史总结为一句话摘要（50-80字）：\n\n");

    for (const ChatMessage &msg : trimmedMessages) {
        QString roleName;
        switch (msg.role) {
        case MessageRole::User:
            roleName = QStringLiteral("用户");
            break;
        case MessageRole::Assistant:
            roleName = QStringLiteral("助手");
            break;
        case MessageRole::System:
            roleName = QStringLiteral("系统");
            break;
        }

        prompt += roleName + QStringLiteral(": ") + msg.content + QStringLiteral("\n");
    }

    return prompt;
}

// 功能：构建 OpenAI-compatible 摘要请求体 JSON。
QByteArray SummaryAPIClient::buildSummaryRequestBody(const QString &prompt) const
{
    QJsonObject message;
    message[QStringLiteral("role")] = QStringLiteral("user");
    message[QStringLiteral("content")] = prompt;

    QJsonArray messages;
    messages.append(message);

    QJsonObject body;
    body[QStringLiteral("model")] = m_config.modelName;
    body[QStringLiteral("messages")] = messages;
    body[QStringLiteral("max_tokens")] = 120;
    body[QStringLiteral("temperature")] = 0.3;

    return QJsonDocument(body).toJson(QJsonDocument::Compact);
}

// 功能：从 API 响应 JSON 中提取 choices[0].message.content 字符串。
QString SummaryAPIClient::extractSummaryFromResponse(const QByteArray &responseBody) const
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(responseBody, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        AppLogger::warning(QStringLiteral("ContextWindow"),
                           QStringLiteral("Summary API response JSON parse failed. error=%1")
                               .arg(parseError.errorString()));
        return QString();
    }

    const QJsonObject root = doc.object();
    const QJsonArray choices = root[QStringLiteral("choices")].toArray();
    if (choices.isEmpty()) {
        AppLogger::warning(QStringLiteral("ContextWindow"),
                           QStringLiteral("Summary API response has no choices array."));
        return QString();
    }

    const QJsonObject firstChoice = choices[0].toObject();
    const QJsonObject message = firstChoice[QStringLiteral("message")].toObject();
    const QString content = message[QStringLiteral("content")].toString();

    return content;
}

// 功能：同步调用 AI API 生成摘要。
// 使用 QEventLoop 实现同步阻塞 HTTP POST，支持超时。
QString SummaryAPIClient::generateSummary(const QVector<ChatMessage> &trimmedMessages)
{
    if (trimmedMessages.isEmpty()) {
        return QString();
    }

    const QString prompt = buildSummaryPrompt(trimmedMessages);
    const QByteArray body = buildSummaryRequestBody(prompt);

    QUrl url(m_config.baseUrl);
    url.setPath(url.path() + QStringLiteral("/chat/completions"));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader(QByteArrayLiteral("Authorization"),
                         QByteArrayLiteral("Bearer ") + m_config.apiKey.toUtf8());

    QNetworkReply *reply = m_networkManager.post(request, body);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    timer.start(m_timeoutMs);
    loop.exec();

    // 超时处理
    if (!timer.isActive()) {
        AppLogger::warning(QStringLiteral("ContextWindow"),
                           QStringLiteral("Summary API request timed out after %1 ms.")
                               .arg(m_timeoutMs));
        reply->abort();
        reply->deleteLater();
        return QString();
    }

    timer.stop();

    if (reply->error() != QNetworkReply::NoError) {
        AppLogger::warning(QStringLiteral("ContextWindow"),
                           QStringLiteral("Summary API request failed. error=%1")
                               .arg(reply->errorString()));
        reply->deleteLater();
        return QString();
    }

    const QByteArray responseBody = reply->readAll();
    reply->deleteLater();

    const QString summary = extractSummaryFromResponse(responseBody);
    if (summary.isEmpty()) {
        AppLogger::warning(QStringLiteral("ContextWindow"),
                           QStringLiteral("Summary API returned empty summary."));
    } else {
        AppLogger::info(QStringLiteral("ContextWindow"),
                        QStringLiteral("Summary generation succeeded. length=%1")
                            .arg(summary.size()));
    }

    return summary;
}
