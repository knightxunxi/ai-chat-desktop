#pragma once

#include "services/PythonSidecarProtocol.h"

#include <QByteArray>
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

private:
    PythonSidecarResponse invalidResponse(const QString &message) const;
    QString nextRequestId();

    QProcess m_process;
    QByteArray m_stdoutBuffer;
    QString m_lastError;
    quint64 m_nextRequestNumber = 0;
};

