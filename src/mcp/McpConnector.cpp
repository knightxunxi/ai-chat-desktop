#include "mcp/McpConnector.h"

#include "tools/ToolResult.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QTimer>
#include <QCoreApplication>

const QString McpConnector::kJsonRpcVersion = QStringLiteral("2.0");

McpConnector::McpConnector(QObject *parent)
    : QObject(parent)
{
}

McpConnector::~McpConnector()
{
    cleanup();
}

bool McpConnector::connectToServer(const QString &command, const QStringList &args)
{
    if (command.isEmpty()) {
        return false;
    }

    // 断开已有连接
    if (m_process) {
        disconnect();
    }

    m_process = new QProcess(this);

    QObject::connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        Q_UNUSED(error);
        if (m_connected) {
            m_connected = false;
            emit serverDisconnected();
        }
    });

    QObject::connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     this, [this](int exitCode, QProcess::ExitStatus status) {
                         Q_UNUSED(exitCode);
                         Q_UNUSED(status);
                         if (m_connected) {
                             m_connected = false;
                             emit serverDisconnected();
                         }
                     });

    m_process->start(command, args);

    // 等待进程启动，10s 超时
    if (!m_process->waitForStarted(10000)) {
        const QString errorMsg = m_process->errorString();
        delete m_process;
        m_process = nullptr;
        emit serverError(QStringLiteral("Failed to start MCP server: %1").arg(errorMsg));
        return false;
    }

    m_connected = true;
    return true;
}

void McpConnector::disconnect()
{
    if (!m_process) {
        return;
    }

    m_connected = false;
    cleanup();
}

bool McpConnector::isConnected() const
{
    return m_connected && m_process != nullptr
           && m_process->state() == QProcess::Running;
}

QVector<McpToolDefinition> McpConnector::listTools()
{
    if (!isConnected()) {
        return {};
    }

    QJsonObject response = sendRequest(QStringLiteral("tools/list"), QJsonObject());

    // 解析响应中的 tools 数组
    if (response.isEmpty()) {
        return {};
    }

    QJsonArray toolsArray;
    // MCP 响应格式: {"result": {"tools": [...]}}
    if (response.contains(QStringLiteral("result"))) {
        QJsonObject resultObj = response.value(QStringLiteral("result")).toObject();
        if (resultObj.contains(QStringLiteral("tools"))) {
            toolsArray = resultObj.value(QStringLiteral("tools")).toArray();
        }
    }

    QVector<McpToolDefinition> tools;
    for (const QJsonValue &val : toolsArray) {
        QJsonObject obj = val.toObject();
        if (obj.isEmpty()) {
            continue;
        }
        McpToolDefinition tool;
        tool.name = obj.value(QStringLiteral("name")).toString();
        tool.description = obj.value(QStringLiteral("description")).toString();
        tool.inputSchema = obj.value(QStringLiteral("inputSchema")).toObject();
        if (!tool.name.isEmpty()) {
            tools.append(tool);
        }
    }
    return tools;
}

ToolResult McpConnector::callTool(const QString &name, const QJsonObject &args)
{
    if (!isConnected()) {
        return ToolResult::failure(QStringLiteral("MCP server not connected"));
    }

    QJsonObject params;
    params[QStringLiteral("name")] = name;
    params[QStringLiteral("arguments")] = args;

    QJsonObject response = sendRequest(QStringLiteral("tools/call"), params);

    if (response.isEmpty()) {
        return ToolResult::failure(QStringLiteral("MCP tools/call: empty response"));
    }

    // 检查是否有 error 字段
    if (response.contains(QStringLiteral("error"))) {
        QJsonObject errorObj = response.value(QStringLiteral("error")).toObject();
        QString errorMsg = errorObj.value(QStringLiteral("message")).toString();
        return ToolResult::failure(QStringLiteral("MCP error: %1").arg(errorMsg));
    }

    // 解析 result.content
    if (response.contains(QStringLiteral("result"))) {
        QJsonObject resultObj = response.value(QStringLiteral("result")).toObject();
        if (resultObj.contains(QStringLiteral("content"))) {
            QJsonArray contentArray = resultObj.value(QStringLiteral("content")).toArray();
            QStringList texts;
            for (const QJsonValue &val : contentArray) {
                QJsonObject contentItem = val.toObject();
                if (contentItem.contains(QStringLiteral("text"))) {
                    texts.append(contentItem.value(QStringLiteral("text")).toString());
                }
            }
            return ToolResult::success(texts.join(QStringLiteral("\n")));
        }
    }

    return ToolResult::success(QString());
}

QVector<McpResource> McpConnector::listResources()
{
    if (!isConnected()) {
        return {};
    }

    QJsonObject response = sendRequest(QStringLiteral("resources/list"), QJsonObject());

    if (response.isEmpty()) {
        return {};
    }

    QJsonArray resourcesArray;
    if (response.contains(QStringLiteral("result"))) {
        QJsonObject resultObj = response.value(QStringLiteral("result")).toObject();
        if (resultObj.contains(QStringLiteral("resources"))) {
            resourcesArray = resultObj.value(QStringLiteral("resources")).toArray();
        }
    }

    QVector<McpResource> resources;
    for (const QJsonValue &val : resourcesArray) {
        QJsonObject obj = val.toObject();
        if (obj.isEmpty()) {
            continue;
        }
        McpResource res;
        res.uri = obj.value(QStringLiteral("uri")).toString();
        res.name = obj.value(QStringLiteral("name")).toString();
        res.description = obj.value(QStringLiteral("description")).toString();
        res.mimeType = obj.value(QStringLiteral("mimeType")).toString();
        if (!res.uri.isEmpty()) {
            resources.append(res);
        }
    }
    return resources;
}

QJsonObject McpConnector::sendRequest(const QString &method, const QJsonObject &params)
{
    if (!m_process) {
        return {};
    }

    const int requestId = m_nextRequestId++;

    QJsonObject request;
    request[QStringLiteral("jsonrpc")] = kJsonRpcVersion;
    request[QStringLiteral("id")] = requestId;
    request[QStringLiteral("method")] = method;
    request[QStringLiteral("params")] = params;

    QJsonDocument doc(request);
    QByteArray requestData = doc.toJson(QJsonDocument::Compact) + QByteArrayLiteral("\n");

    if (m_process->write(requestData) == -1) {
        m_connected = false;
        emit serverDisconnected();
        return {};
    }

    return readResponse();
}

QJsonObject McpConnector::readResponse()
{
    if (!m_process) {
        return {};
    }

    // tools/call 用 30s 超时，其余用 10s 超时
    // 这里统一使用 30s 超时等待可读数据
    if (!m_process->waitForReadyRead(30000)) {
        emit serverError(QStringLiteral("MCP response timeout"));
        return {};
    }

    QByteArray data = m_process->readLine();
    if (data.isEmpty()) {
        return {};
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data.trimmed(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        emit serverError(QStringLiteral("MCP JSON parse error: %1").arg(parseError.errorString()));
        return {};
    }

    return doc.object();
}

void McpConnector::cleanup()
{
    if (m_process) {
        m_process->disconnect();
        if (m_process->state() != QProcess::NotRunning) {
            m_process->terminate();
            if (!m_process->waitForFinished(3000)) {
                m_process->kill();
                m_process->waitForFinished(2000);
            }
        }
        delete m_process;
        m_process = nullptr;
    }
    m_connected = false;
    m_nextRequestId = 1;
}
