#include "MockApiClient.h"

MockApiClient::MockApiClient(QObject *parent) : AIClient(parent) {}

void MockApiClient::setResponses(const QVector<MockResponseUnit> &responses)
{ m_responses = responses; m_responseIndex = 0; m_requestCount = 0; m_cancelled = false; }

int MockApiClient::remainingResponses() const
{ return m_responses.size() - m_responseIndex; }

void MockApiClient::sendChat(const AppConfig &, const ChatSession &) { processNextResponse(); }

void MockApiClient::sendChatWithTools(const AppConfig &, const ChatSession &, const QJsonArray &tools)
{ m_lastTools = tools; processNextResponse(); }

void MockApiClient::sendChatWithImages(const AppConfig &, const ChatSession &, const QJsonArray &, const QJsonArray &)
{ processNextResponse(); }

void MockApiClient::cancel() { m_cancelled = true; }

void MockApiClient::processNextResponse()
{
    m_requestCount++;
    if (m_responseIndex >= m_responses.size()) {
        emit requestFailed(QStringLiteral("Mock: no more responses"), RequestErrorCategory::Unknown);
        return;
    }
    const MockResponseUnit &resp = m_responses[m_responseIndex++];
    if (resp.isError) {
        emit requestFailed(resp.errorMessage, resp.errorCategory);
        return;
    }
    if (!resp.textDelta.isEmpty()) emit textDeltaReceived(resp.textDelta);
    if (!resp.toolCalls.isEmpty()) emit toolCallsReceived(resp.toolCalls);
    emit requestFinished();
}
