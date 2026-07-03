#include "services/PythonSidecarClient.h"

#include <QElapsedTimer>

namespace {

int remainingTime(const QElapsedTimer &timer, int timeoutMs)
{
    return qMax(0, timeoutMs - static_cast<int>(timer.elapsed()));
}

} // namespace

PythonSidecarClient::~PythonSidecarClient()
{
    stop();
}

bool PythonSidecarClient::start(const PythonSidecarStartOptions &options, int timeoutMs)
{
    stop();
    m_lastError.clear();
    m_stdoutBuffer.clear();

    if (options.program.trimmed().isEmpty()) {
        m_lastError = QStringLiteral("Python sidecar program is empty.");
        return false;
    }

    m_process.setProgram(options.program);
    m_process.setArguments(options.arguments);
    if (!options.workingDirectory.trimmed().isEmpty()) {
        m_process.setWorkingDirectory(options.workingDirectory);
    }
    m_process.setProcessEnvironment(options.environment);
    m_process.setProcessChannelMode(QProcess::SeparateChannels);
    m_process.start();

    if (!m_process.waitForStarted(timeoutMs)) {
        m_lastError = m_process.errorString();
        return false;
    }

    return true;
}

void PythonSidecarClient::stop()
{
    if (m_process.state() == QProcess::NotRunning) {
        return;
    }

    m_process.closeWriteChannel();
    if (!m_process.waitForFinished(1000)) {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
}

bool PythonSidecarClient::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

PythonSidecarResponse PythonSidecarClient::send(const QString &method, const QJsonObject &params, int timeoutMs)
{
    if (!isRunning()) {
        return invalidResponse(QStringLiteral("Python sidecar process is not running."));
    }

    const QString requestId = nextRequestId();
    const QByteArray request = PythonSidecarProtocol::buildRequest(requestId, method, params);
    if (m_process.write(request) != request.size()) {
        return invalidResponse(QStringLiteral("Failed to write request to Python sidecar."));
    }
    if (!m_process.waitForBytesWritten(timeoutMs)) {
        return invalidResponse(QStringLiteral("Timed out while writing request to Python sidecar."));
    }

    QElapsedTimer timer;
    timer.start();
    while (remainingTime(timer, timeoutMs) > 0) {
        m_stdoutBuffer.append(m_process.readAllStandardOutput());
        const int newlineIndex = m_stdoutBuffer.indexOf('\n');
        if (newlineIndex >= 0) {
            const QByteArray line = m_stdoutBuffer.left(newlineIndex);
            m_stdoutBuffer.remove(0, newlineIndex + 1);

            PythonSidecarResponse response = PythonSidecarProtocol::parseResponse(line);
            if (response.id.isEmpty() || response.id == requestId) {
                return response;
            }
            continue;
        }

        if (m_process.state() == QProcess::NotRunning) {
            const QString stderrText = QString::fromUtf8(m_process.readAllStandardError()).trimmed();
            return invalidResponse(stderrText.isEmpty()
                                       ? QStringLiteral("Python sidecar exited before responding.")
                                       : stderrText);
        }

        m_process.waitForReadyRead(qMin(remainingTime(timer, timeoutMs), 100));
    }

    return invalidResponse(QStringLiteral("Python sidecar request timed out."));
}

QString PythonSidecarClient::lastError() const
{
    return m_lastError;
}

PythonSidecarResponse PythonSidecarClient::invalidResponse(const QString &message) const
{
    PythonSidecarResponse response;
    response.parseError = message;
    return response;
}

QString PythonSidecarClient::nextRequestId()
{
    ++m_nextRequestNumber;
    return QStringLiteral("cpp-%1").arg(m_nextRequestNumber);
}

