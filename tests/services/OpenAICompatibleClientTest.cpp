#include "services/OpenAICompatibleClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <cassert>

int main()
{
    AppConfig config = AppConfig::defaultConfig();
    config.modelName = QStringLiteral("test-model");
    config.apiKey = QStringLiteral("test-key");

    ChatSession session = ChatSession::createDefault();
    session.systemPrompt = QStringLiteral("You are a concise C++ mentor.");
    session.addMessage(MessageRole::System, QStringLiteral("This stored system message should be ignored."));
    session.addMessage(MessageRole::User, QStringLiteral("Explain RAII."));
    session.addMessage(MessageRole::Assistant, QStringLiteral("RAII ties resource lifetime to object lifetime."));
    session.addMessage(MessageRole::Assistant, QString());

    const QJsonDocument document = QJsonDocument::fromJson(OpenAICompatibleClient::buildRequestBody(config, session));
    assert(document.isObject());

    const QJsonObject body = document.object();
    assert(body.value(QStringLiteral("model")).toString() == QStringLiteral("test-model"));
    assert(body.value(QStringLiteral("stream")).toBool());

    const QJsonArray messages = body.value(QStringLiteral("messages")).toArray();
    assert(messages.size() == 3);
    assert(messages[0].toObject().value(QStringLiteral("role")).toString() == QStringLiteral("system"));
    assert(messages[0].toObject().value(QStringLiteral("content")).toString() == QStringLiteral("You are a concise C++ mentor."));
    assert(messages[1].toObject().value(QStringLiteral("role")).toString() == QStringLiteral("user"));
    assert(messages[1].toObject().value(QStringLiteral("content")).toString() == QStringLiteral("Explain RAII."));
    assert(messages[2].toObject().value(QStringLiteral("role")).toString() == QStringLiteral("assistant"));

    return 0;
}
