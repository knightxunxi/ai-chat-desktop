#include "core/AppConfig.h"
#include "core/ChatSession.h"

#include <cassert>

int main()
{
    AppConfig config = AppConfig::defaultConfig();
    assert(config.providerName == QStringLiteral("DeepSeek"));
    assert(config.baseUrl == QStringLiteral("https://api.deepseek.com"));
    assert(config.modelName == QStringLiteral("deepseek-v4-flash"));
    assert(config.language == AppLanguage::Chinese);
    assert(!config.temperature.has_value());
    assert(!config.maxTokens.has_value());
    assert(!config.isComplete());

    config.apiKey = QStringLiteral("test-key");
    assert(config.isComplete());
    assert(appLanguageFromString(QStringLiteral("en_US")) == AppLanguage::English);
    assert(appLanguageToString(AppLanguage::Chinese) == QStringLiteral("zh_CN"));

    ChatSession session = ChatSession::createDefault();
    assert(!session.id.isEmpty());
    assert(session.title == QStringLiteral("New Chat"));
    assert(!session.hasSystemPrompt());

    session.systemPrompt = QStringLiteral("You are a C++ tutor.");
    assert(session.hasSystemPrompt());

    const ChatMessage userMessage = session.addMessage(MessageRole::User, QStringLiteral("Hello"));
    const ChatMessage assistantMessage = session.addMessage(MessageRole::Assistant, QStringLiteral("Hi"));

    assert(session.messages.size() == 2);
    assert(userMessage.sessionId == session.id);
    assert(userMessage.role == MessageRole::User);
    assert(assistantMessage.role == MessageRole::Assistant);
    assert(messageRoleToString(MessageRole::System) == QStringLiteral("system"));
    assert(messageRoleFromString(QStringLiteral("assistant")) == MessageRole::Assistant);

    return 0;
}
