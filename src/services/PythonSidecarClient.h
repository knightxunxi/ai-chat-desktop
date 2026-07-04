#pragma once

// 功能：Python sidecar QProcess 客户端封装。
// 生命周期：start() → send()/listProviders() → stop()，两次 start 间自动 stop。
// 使用模块：PythonSidecarAIClient 和 AgentToolRegistry (system.list_providers 工具)。

#include "services/PythonSidecarProtocol.h"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

struct PythonSidecarStartOptions {
    QString program;
    QStringList arguments;
    QString workingDirectory;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
};

class PythonSidecarClient
{
public:
    ~PythonSidecarClient();

    bool start(const PythonSidecarStartOptions &options, int timeoutMs = 3000);
    void stop();
    bool isRunning() const;

    PythonSidecarResponse send(const QString &method, const QJsonObject &params = QJsonObject(), int timeoutMs = 10000);
    QString lastError() const;

    // V19 #16: 列出 Python sidecar 中配置的 AI 厂商列表
    QJsonArray listProviders(int timeoutMs = 5000);

private:
    PythonSidecarResponse invalidResponse(const QString &message) const;
    QString nextRequestId();

    QProcess m_process;
    QByteArray m_stdoutBuffer;
    QString m_lastError;
    quint64 m_nextRequestNumber = 0;
};

