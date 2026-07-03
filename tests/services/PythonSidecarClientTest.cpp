#include "services/PythonSidecarClient.h"

#include <QCoreApplication>
#include <QJsonObject>
#include <QProcessEnvironment>

#include <cassert>

#ifndef CODEXX_PYTHON_EXECUTABLE
#define CODEXX_PYTHON_EXECUTABLE ""
#endif

#ifndef CODEXX_PYTHON_SIDECAR_DIR
#define CODEXX_PYTHON_SIDECAR_DIR ""
#endif

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("PYTHONPATH"), QStringLiteral(CODEXX_PYTHON_SIDECAR_DIR));

    PythonSidecarStartOptions options;
    options.program = QStringLiteral(CODEXX_PYTHON_EXECUTABLE);
    options.arguments = QStringList{QStringLiteral("-m"), QStringLiteral("agent_sidecar")};
    options.workingDirectory = QStringLiteral(CODEXX_PYTHON_SIDECAR_DIR);
    options.environment = environment;

    PythonSidecarClient client;
    assert(client.start(options, 5000));
    assert(client.isRunning());

    const PythonSidecarResponse ping = client.send(QStringLiteral("ping"), QJsonObject(), 5000);
    assert(ping.valid);
    assert(ping.ok);
    assert(ping.result.value(QStringLiteral("status")).toString() == QStringLiteral("ok"));

    const PythonSidecarResponse tokenCount = client.send(
        QStringLiteral("token.count"),
        QJsonObject{{QStringLiteral("text"), QStringLiteral("hello 世界")}},
        5000);
    assert(tokenCount.valid);
    assert(tokenCount.ok);
    assert(tokenCount.result.value(QStringLiteral("tokens")).toInt() > 0);

    const PythonSidecarResponse missingMethod = client.send(QStringLiteral("missing.method"), QJsonObject(), 5000);
    assert(missingMethod.valid);
    assert(!missingMethod.ok);
    assert(missingMethod.error.code == QStringLiteral("method_not_found"));

    client.stop();
    assert(!client.isRunning());

    return 0;
}

