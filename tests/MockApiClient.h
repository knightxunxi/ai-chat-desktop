#pragma once

#include "services/AIClient.h"
#include "services/ToolCall.h"

#include <QJsonArray>
#include <QVector>

struct MockResponseUnit {
    QString textDelta;
    ToolCallList toolCalls;
    bool isTruncated = false;
    bool isError = false;
    RequestErrorCategory errorCategory = RequestErrorCategory::Unknown;
    QString errorMessage;
};

class MockApiClient : public AIClient
{
    Q_OBJECT
public:
    explicit MockApiClient(QObject *parent = nullptr);
    void setResponses(const QVector<MockResponseUnit> &responses);
    int remainingResponses() const;

    void sendChat(const AppConfig &config, const ChatSession &session) override;
    void sendChatWithTools(const AppConfig &config, const ChatSession &session, const QJsonArray &tools) override;
    void sendChatWithImages(const AppConfig &config, const ChatSession &session, const QJsonArray &tools, const QJsonArray &images) override;
    void cancel() override;

    int requestCount() const { return m_requestCount; }
    bool wasCancelled() const { return m_cancelled; }
    const QJsonArray &lastTools() const { return m_lastTools; }

private:
    void processNextResponse();
    QVector<MockResponseUnit> m_responses;
    int m_responseIndex = 0;
    int m_requestCount = 0;
    bool m_cancelled = false;
    QJsonArray m_lastTools;
};
