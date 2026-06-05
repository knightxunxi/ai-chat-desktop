#include "services/OpenAICompatibleClient.h"

#include "support/AppLogger.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace {

int requestMessageCount(const ChatSession &session)
{
    int count = session.hasSystemPrompt() ? 1 : 0;
    for (const ChatMessage &message : session.messages) {
        if (message.role != MessageRole::System && !message.content.trimmed().isEmpty()) {
            ++count;
        }
    }

    return count;
}

QString yesNo(bool value)
{
    return value ? QStringLiteral("yes") : QStringLiteral("no");
}

RequestErrorCategory classifyNetworkError(QNetworkReply::NetworkError error)
{
    return error == QNetworkReply::NoError ? RequestErrorCategory::Unknown : RequestErrorCategory::Network;
}

QString errorCategoryName(RequestErrorCategory category)
{
    switch (category) {
    case RequestErrorCategory::Network:
        return QStringLiteral("network");
    case RequestErrorCategory::Authentication:
        return QStringLiteral("authentication");
    case RequestErrorCategory::Quota:
        return QStringLiteral("quota");
    case RequestErrorCategory::Model:
        return QStringLiteral("model");
    case RequestErrorCategory::Server:
        return QStringLiteral("server");
    case RequestErrorCategory::Unknown:
        return QStringLiteral("unknown");
    }

    return QStringLiteral("unknown");
}

} // namespace

OpenAICompatibleClient::OpenAICompatibleClient(QObject *parent)
    : AIClient(parent)
{
}

void OpenAICompatibleClient::sendChat(const AppConfig &config, const ChatSession &session)
{
    sendChatWithTools(config, session, QJsonArray());
}

void OpenAICompatibleClient::sendChatWithTools(const AppConfig &config, const ChatSession &session, const QJsonArray &tools)
{
    cancel();

    m_streamParser.reset();
    m_errorBody.clear();
    m_doneReceived = false;
    m_streamWasTruncated = false;

    const QUrl requestUrl = chatCompletionsUrl(config.baseUrl);
    AppLogger::info(QStringLiteral("AIClient"),
                    QStringLiteral("Chat request started. url=%1 model=%2 messages=%3 systemPrompt=%4")
                        .arg(requestUrl.toString(), config.modelName,
                             QString::number(requestMessageCount(session)),
                             yesNo(session.hasSystemPrompt())));

    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(config.apiKey).toUtf8());

    m_currentReply = m_networkManager.post(request, buildRequestBody(config, session, tools));

    connect(m_currentReply, &QNetworkReply::readyRead, this, &OpenAICompatibleClient::handleReadyRead);
    connect(m_currentReply, &QNetworkReply::finished, this, &OpenAICompatibleClient::handleFinished);
}

void OpenAICompatibleClient::sendChatWithImages(const AppConfig &config, const ChatSession &session,
                                                  const QJsonArray &tools, const QJsonArray &images)
{
    cancel();

    m_streamParser.reset();
    m_errorBody.clear();
    m_doneReceived = false;
    m_streamWasTruncated = false;

    const QUrl requestUrl = chatCompletionsUrl(config.baseUrl);
    AppLogger::info(QStringLiteral("AIClient"),
                    QStringLiteral("Chat request with images started. url=%1 model=%2 messages=%3 images=%4")
                        .arg(requestUrl.toString(), config.modelName,
                             QString::number(requestMessageCount(session)),
                             QString::number(images.size())));

    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(config.apiKey).toUtf8());

    m_currentReply = m_networkManager.post(request, buildRequestBody(config, session, tools, images));

    connect(m_currentReply, &QNetworkReply::readyRead, this, &OpenAICompatibleClient::handleReadyRead);
    connect(m_currentReply, &QNetworkReply::finished, this, &OpenAICompatibleClient::handleFinished);
}

void OpenAICompatibleClient::cancel()
{
    m_streamWasTruncated = false;
    if (m_currentReply == nullptr) {
        return;
    }

    AppLogger::info(QStringLiteral("AIClient"),
                    QStringLiteral("Chat request canceled. url=%1").arg(m_currentReply->url().toString()));

    disconnect(m_currentReply, nullptr, this, nullptr);
    m_currentReply->abort();
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
    m_streamParser.reset();
    m_errorBody.clear();
    m_doneReceived = false;
    m_streamWasTruncated = false;
}

RequestErrorCategory OpenAICompatibleClient::classifyHttpStatus(int statusCode)
{
    if (statusCode == 401 || statusCode == 403) {
        return RequestErrorCategory::Authentication;
    }
    if (statusCode == 402 || statusCode == 429) {
        return RequestErrorCategory::Quota;
    }
    if (statusCode == 400 || statusCode == 404 || statusCode == 422) {
        return RequestErrorCategory::Model;
    }
    if (statusCode >= 500) {
        return RequestErrorCategory::Server;
    }

    return RequestErrorCategory::Unknown;
}

QUrl OpenAICompatibleClient::chatCompletionsUrl(const QString &baseUrl) const
{
    QString normalized = baseUrl.trimmed();
    while (normalized.endsWith(QLatin1Char('/'))) {
        normalized.chop(1);
    }

    if (!normalized.endsWith(QStringLiteral("/chat/completions"))) {
        normalized += QStringLiteral("/chat/completions");
    }

    return QUrl(normalized);
}

QByteArray OpenAICompatibleClient::buildRequestBody(const AppConfig &config, const ChatSession &session, const QJsonArray &tools)
{
    QJsonArray messages;

    if (session.hasSystemPrompt()) {
        QJsonObject systemMessage;
        systemMessage.insert(QStringLiteral("role"), QStringLiteral("system"));
        systemMessage.insert(QStringLiteral("content"), session.systemPrompt.trimmed());
        messages.append(systemMessage);
    }

    for (const ChatMessage &message : session.messages) {
        if (message.role == MessageRole::System) {
            continue;
        }

        if (message.content.trimmed().isEmpty()) {
            continue;
        }

        QJsonObject messageObject;
        messageObject.insert(QStringLiteral("role"), messageRoleToString(message.role));
        messageObject.insert(QStringLiteral("content"), message.content);
        messages.append(messageObject);
    }

    QJsonObject body;
    body.insert(QStringLiteral("model"), config.modelName);
    body.insert(QStringLiteral("messages"), messages);
    body.insert(QStringLiteral("stream"), true);
    if (config.temperature.has_value()) {
        body.insert(QStringLiteral("temperature"), config.temperature.value());
    }
    if (config.maxTokens.has_value()) {
        body.insert(QStringLiteral("max_tokens"), config.maxTokens.value());
    }
    if (!tools.isEmpty()) {
        body.insert(QStringLiteral("tools"), tools);
        body.insert(QStringLiteral("tool_choice"), QStringLiteral("auto"));
    }

    return QJsonDocument(body).toJson(QJsonDocument::Compact);
}

QByteArray OpenAICompatibleClient::buildRequestBody(const AppConfig &config, const ChatSession &session,
                                                     const QJsonArray &tools, const QJsonArray &images)
{
    QJsonArray messages;

    if (session.hasSystemPrompt()) {
        QJsonObject systemMessage;
        systemMessage.insert(QStringLiteral("role"), QStringLiteral("system"));
        systemMessage.insert(QStringLiteral("content"), session.systemPrompt.trimmed());
        messages.append(systemMessage);
    }

    // V17.1-fix: 扫描找到最后一条 User 消息的索引（而非依赖最后一条消息，
    // 因为 ApplicationController 可能在发送前追加了空的 Assistant 占位消息）
    int lastUserIndex = -1;
    for (int i = 0; i < session.messages.size(); ++i) {
        if (session.messages[i].role == MessageRole::User
            && !session.messages[i].content.trimmed().isEmpty()) {
            lastUserIndex = i;
        }
    }

    int msgIndex = -1;
    for (const ChatMessage &message : session.messages) {
        ++msgIndex;

        if (message.role == MessageRole::System) {
            continue;
        }

        if (message.content.trimmed().isEmpty()) {
            continue;
        }

        // 最后一条 User 消息如果带了图片，用多模态 content 格式
        bool isLastUserMessage = (msgIndex == lastUserIndex
                                  && message.role == MessageRole::User);

        if (isLastUserMessage && !images.isEmpty()) {
            QJsonObject msgObj;
            msgObj.insert(QStringLiteral("role"), QStringLiteral("user"));
            QJsonArray contentArray;

            // 文本部分
            QJsonObject textPart;
            textPart.insert(QStringLiteral("type"), QStringLiteral("text"));
            textPart.insert(QStringLiteral("text"), message.content);
            contentArray.append(textPart);

            // 图片部分 — DeepSeek 格式：{type: "image", image: {data: "<base64>", format: "base64"}}
            for (const QJsonValue &img : images) {
                // 剥离 "data:image/png;base64," 前缀，只保留原始 base64
                QString rawBase64 = img.toString();
                static const QString kBase64Prefix = QStringLiteral("data:image/png;base64,");
                if (rawBase64.startsWith(kBase64Prefix)) {
                    rawBase64 = rawBase64.mid(kBase64Prefix.length());
                }

                QJsonObject imgPart;
                imgPart.insert(QStringLiteral("type"), QStringLiteral("image"));
                QJsonObject imgData;
                imgData.insert(QStringLiteral("data"), rawBase64);
                imgData.insert(QStringLiteral("format"), QStringLiteral("base64"));
                imgPart.insert(QStringLiteral("image"), imgData);
                contentArray.append(imgPart);
            }

            msgObj.insert(QStringLiteral("content"), contentArray);
            messages.append(msgObj);
        } else {
            QJsonObject msgObj;
            msgObj.insert(QStringLiteral("role"), messageRoleToString(message.role));
            msgObj.insert(QStringLiteral("content"), message.content);
            messages.append(msgObj);
        }
    }

    QJsonObject body;
    body.insert(QStringLiteral("model"), config.modelName);
    body.insert(QStringLiteral("messages"), messages);
    body.insert(QStringLiteral("stream"), true);
    if (config.temperature.has_value()) {
        body.insert(QStringLiteral("temperature"), config.temperature.value());
    }
    if (config.maxTokens.has_value()) {
        body.insert(QStringLiteral("max_tokens"), config.maxTokens.value());
    }
    if (!tools.isEmpty()) {
        body.insert(QStringLiteral("tools"), tools);
        body.insert(QStringLiteral("tool_choice"), QStringLiteral("auto"));
    }

    return QJsonDocument(body).toJson(QJsonDocument::Compact);
}

QString OpenAICompatibleClient::extractErrorMessage(const QByteArray &body, const QString &fallback) const
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return fallback;
    }

    const QJsonValue errorValue = document.object().value(QStringLiteral("error"));
    if (errorValue.isObject()) {
        const QString message = errorValue.toObject().value(QStringLiteral("message")).toString();
        if (!message.isEmpty()) {
            return message;
        }
    }

    return fallback;
}

void OpenAICompatibleClient::handleReadyRead()
{
    if (m_currentReply == nullptr) {
        return;
    }

    const QByteArray data = m_currentReply->readAll();
    const int statusCode = m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode >= 400) {
        m_errorBody.append(data);
        return;
    }

    const StreamParseResult result = m_streamParser.consume(data);
    // V17.6 P2-2: 追踪截断标记
    if (result.truncated) {
        m_streamWasTruncated = true;
    }
    for (const QString &delta : result.textDeltas) {
        emit textDeltaReceived(delta);
    }
    if (!result.toolCalls.isEmpty()) {
        emit toolCallsReceived(result.toolCalls);
    }
    // V12.3: 流式工具调用参数完整时立即通知上层
    for (const auto &event : result.blockEvents) {
        if (event.type == ContentBlockEventType::ToolUseComplete) {
            emit toolUseBlockComplete(event.toolName, event.arguments);
        }
    }

    if (result.done) {
        m_doneReceived = true;
    }
}

void OpenAICompatibleClient::handleFinished()
{
    if (m_currentReply == nullptr) {
        return;
    }

    QNetworkReply *reply = m_currentReply;
    m_currentReply = nullptr;

    const QByteArray remainingBody = reply->readAll();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (statusCode >= 400) {
        m_errorBody.append(remainingBody);
        const QString fallback = QStringLiteral("HTTP request failed with status %1.").arg(statusCode);
        const QString errorMessage = extractErrorMessage(m_errorBody, fallback);
        const RequestErrorCategory category = classifyHttpStatus(statusCode);
        AppLogger::error(QStringLiteral("AIClient"),
                         QStringLiteral("Chat request failed. url=%1 status=%2 category=%3 error=%4")
                             .arg(reply->url().toString(), QString::number(statusCode), errorCategoryName(category), errorMessage));
        emit requestFailed(errorMessage, category);
    } else if (reply->error() != QNetworkReply::NoError) {
        m_errorBody.append(remainingBody);
        const QString errorMessage = extractErrorMessage(m_errorBody, reply->errorString());
        const RequestErrorCategory category = classifyNetworkError(reply->error());
        AppLogger::error(QStringLiteral("AIClient"),
                         QStringLiteral("Chat request network error. url=%1 status=%2 networkError=%3 category=%4 error=%5")
                             .arg(reply->url().toString(), QString::number(statusCode), QString::number(reply->error()), errorCategoryName(category), errorMessage));
        emit requestFailed(errorMessage, category);
    } else {
        if (!remainingBody.isEmpty()) {
            const StreamParseResult result = m_streamParser.consume(remainingBody);
            if (result.truncated) {
                m_streamWasTruncated = true;
            }
            for (const QString &delta : result.textDeltas) {
                emit textDeltaReceived(delta);
            }
            if (!result.toolCalls.isEmpty()) {
                emit toolCallsReceived(result.toolCalls);
            }
            // V12.3: blockEvents from remaining body
            for (const auto &event : result.blockEvents) {
                if (event.type == ContentBlockEventType::ToolUseComplete) {
                    emit toolUseBlockComplete(event.toolName, event.arguments);
                }
            }
            if (result.done) {
                m_doneReceived = true;
            }
        }

        const StreamParseResult finalResult = m_streamParser.finish();
        if (finalResult.truncated) {
            m_streamWasTruncated = true;
        }
        for (const QString &delta : finalResult.textDeltas) {
            emit textDeltaReceived(delta);
        }
        if (!finalResult.toolCalls.isEmpty()) {
            emit toolCallsReceived(finalResult.toolCalls);
        }
        // V12.3: blockEvents from finish result
        for (const auto &event : finalResult.blockEvents) {
            if (event.type == ContentBlockEventType::ToolUseComplete) {
                emit toolUseBlockComplete(event.toolName, event.arguments);
            }
        }
        if (finalResult.done) {
            m_doneReceived = true;
        }

        if (!m_doneReceived) {
            m_streamWasTruncated = false;
            const QString errorMessage = QStringLiteral("Streaming response ended before the completion marker.");
            AppLogger::warning(QStringLiteral("AIClient"),
                               QStringLiteral("Chat request stream ended unexpectedly. url=%1 status=%2")
                                   .arg(reply->url().toString(), QString::number(statusCode)));
            emit requestFailed(errorMessage, RequestErrorCategory::Server);
        } else {
            AppLogger::info(QStringLiteral("AIClient"),
                            QStringLiteral("Chat request finished. url=%1 status=%2")
                                .arg(reply->url().toString(), QString::number(statusCode)));
            const bool wasTruncated = m_streamWasTruncated;
            m_streamWasTruncated = false;
            // V17.6 P2-2: 先通知控制层记录截断，再由 requestFinished 统一收尾。
            if (wasTruncated) {
                AppLogger::warning(QStringLiteral("AIClient"), QStringLiteral("Response was truncated (finish_reason=length)"));
                emit responseTruncated();
            }
            emit requestFinished();
        }
    }

    reply->deleteLater();
    m_streamParser.reset();
    m_errorBody.clear();
    m_doneReceived = false;
    m_streamWasTruncated = false;
}
