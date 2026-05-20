#include "services/OpenAICompatibleClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

OpenAICompatibleClient::OpenAICompatibleClient(QObject *parent)
    : AIClient(parent)
{
}

void OpenAICompatibleClient::sendChat(const AppConfig &config, const ChatSession &session)
{
    cancel();

    m_streamParser.reset();
    m_errorBody.clear();
    m_doneReceived = false;

    QNetworkRequest request(chatCompletionsUrl(config.baseUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(config.apiKey).toUtf8());

    m_currentReply = m_networkManager.post(request, buildRequestBody(config, session));

    connect(m_currentReply, &QNetworkReply::readyRead, this, &OpenAICompatibleClient::handleReadyRead);
    connect(m_currentReply, &QNetworkReply::finished, this, &OpenAICompatibleClient::handleFinished);
}

void OpenAICompatibleClient::cancel()
{
    if (m_currentReply == nullptr) {
        return;
    }

    disconnect(m_currentReply, nullptr, this, nullptr);
    m_currentReply->abort();
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
    m_streamParser.reset();
    m_errorBody.clear();
    m_doneReceived = false;
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

QByteArray OpenAICompatibleClient::buildRequestBody(const AppConfig &config, const ChatSession &session)
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
    for (const QString &delta : result.textDeltas) {
        emit textDeltaReceived(delta);
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
        emit requestFailed(extractErrorMessage(m_errorBody, fallback));
    } else if (reply->error() != QNetworkReply::NoError) {
        m_errorBody.append(remainingBody);
        emit requestFailed(extractErrorMessage(m_errorBody, reply->errorString()));
    } else {
        if (!remainingBody.isEmpty()) {
            const StreamParseResult result = m_streamParser.consume(remainingBody);
            for (const QString &delta : result.textDeltas) {
                emit textDeltaReceived(delta);
            }
            if (result.done) {
                m_doneReceived = true;
            }
        }

        const StreamParseResult finalResult = m_streamParser.finish();
        for (const QString &delta : finalResult.textDeltas) {
            emit textDeltaReceived(delta);
        }
        if (finalResult.done) {
            m_doneReceived = true;
        }

        if (!m_doneReceived) {
            emit requestFailed(QStringLiteral("Streaming response ended before the completion marker."));
        } else {
            emit requestFinished();
        }
    }

    reply->deleteLater();
    m_streamParser.reset();
    m_errorBody.clear();
    m_doneReceived = false;
}
