#include "services/PythonSidecarAIClient.h"

#include <QJsonArray>

PythonSidecarAIClient::PythonSidecarAIClient(QObject *parent)
    : AIClient(parent)
{
}

PythonSidecarAIClient::~PythonSidecarAIClient()
{
    stopSidecar();
}

bool PythonSidecarAIClient::startSidecar(const QString &pythonExecutable,
                                          const QString &sidecarDir, int timeoutMs)
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("PYTHONPATH"), sidecarDir);

    PythonSidecarStartOptions opts;
    opts.program = pythonExecutable;
    opts.arguments = {QStringLiteral("-m"), QStringLiteral("agent_sidecar")};
    opts.workingDirectory = sidecarDir;
    opts.environment = env;

    return m_client.start(opts, timeoutMs);
}

bool PythonSidecarAIClient::isSidecarRunning() const
{
    return m_client.isRunning();
}

void PythonSidecarAIClient::stopSidecar()
{
    m_client.stop();
}

QString PythonSidecarAIClient::lastError() const
{
    return m_client.lastError();
}

PythonSidecarClient *PythonSidecarAIClient::sidecarClient()
{
    return &m_client;
}

const PythonSidecarClient *PythonSidecarAIClient::sidecarClient() const
{
    return &m_client;
}

void PythonSidecarAIClient::cancel()
{
    m_cancelled = true;
    stopSidecar();
}

void PythonSidecarAIClient::sendChat(const AppConfig &config, const ChatSession &session)
{
    sendChatWithTools(config, session, QJsonArray());
}

void PythonSidecarAIClient::sendChatWithTools(const AppConfig &config,
                                               const ChatSession &session,
                                               const QJsonArray &tools)
{
    sendChatWithImages(config, session, tools, QJsonArray());
}

void PythonSidecarAIClient::sendChatWithImages(const AppConfig &config,
                                                const ChatSession &session,
                                                const QJsonArray &tools,
                                                const QJsonArray &images)
{
    Q_UNUSED(images);
    m_cancelled = false;

    if (!isSidecarRunning()) {
        emit requestFailed(QStringLiteral("Python sidecar is not running."), RequestErrorCategory::Network);
        return;
    }

    const QJsonArray messages = buildMessageArray(session);
    doChatRequest(config, messages, tools, QJsonArray());
}

void PythonSidecarAIClient::doChatRequest(const AppConfig &config,
                                           const QJsonArray &messages,
                                           const QJsonArray &tools,
                                           const QJsonArray & /*images*/)
{
    QJsonObject params;
    params[QStringLiteral("base_url")] = config.baseUrl;
    params[QStringLiteral("model")] = config.modelName;
    params[QStringLiteral("messages")] = messages;
    params[QStringLiteral("api_key")] = config.apiKey;

    if (!tools.isEmpty()) {
        params[QStringLiteral("tools")] = tools;
    }

    // 可选参数
    if (config.temperature.has_value()) {
        params[QStringLiteral("temperature")] = config.temperature.value();
    }
    if (config.maxTokens.has_value()) {
        params[QStringLiteral("max_tokens")] = config.maxTokens.value();
    }

    // stream=False 模式发送
    const PythonSidecarResponse resp = m_client.send(
        QStringLiteral("model.chat"), params, 60000);

    if (!resp.valid) {
        emit requestFailed(resp.parseError, RequestErrorCategory::Network);
        return;
    }

    if (!resp.ok) {
        const bool retryable = resp.error.retryable;
        const RequestErrorCategory cat = retryable ? RequestErrorCategory::Server
                                                   : RequestErrorCategory::Unknown;
        emit requestFailed(resp.error.message, cat);
        return;
    }

    // 解析结果
    const QString text = resp.result.value(QStringLiteral("text")).toString();
    if (!text.isEmpty()) {
        emit textDeltaReceived(text);
    }

    // 处理 tool_calls
    const QJsonValue tcVal = resp.result.value(QStringLiteral("tool_calls"));
    if (!tcVal.isNull() && tcVal.isArray()) {
        ToolCallList toolCalls;
        const QJsonArray tcArr = tcVal.toArray();
        for (const QJsonValue &v : tcArr) {
            const QJsonObject tcObj = v.toObject();
            ToolCall tc;
            tc.id = tcObj.value(QStringLiteral("id")).toString();
            const QJsonObject func = tcObj.value(QStringLiteral("function")).toObject();
            tc.functionName = func.value(QStringLiteral("name")).toString();
            tc.arguments = func.value(QStringLiteral("arguments")).toString();
            toolCalls.append(tc);
        }
        if (!toolCalls.isEmpty()) {
            emit toolCallsReceived(toolCalls);
        }
    }

    // 触发完成
    emit requestFinished();
}

QJsonArray PythonSidecarAIClient::buildMessageArray(const ChatSession &session) const
{
    QJsonArray messages;
    for (const ChatMessage &msg : session.messages) {
        QJsonObject entry;
        switch (msg.role) {
        case MessageRole::System:
            entry[QStringLiteral("role")] = QStringLiteral("system");
            break;
        case MessageRole::User:
            entry[QStringLiteral("role")] = QStringLiteral("user");
            break;
        case MessageRole::Assistant:
            entry[QStringLiteral("role")] = QStringLiteral("assistant");
            break;
        }
        entry[QStringLiteral("content")] = msg.content;
        messages.append(entry);
    }
    return messages;
}
