#include "services/PythonSidecarProtocol.h"

#include <QJsonDocument>
#include <QJsonParseError>

QByteArray PythonSidecarProtocol::buildRequest(const QString &id, const QString &method, const QJsonObject &params)
{
    QJsonObject request;
    request.insert(QStringLiteral("id"), id);
    request.insert(QStringLiteral("method"), method);
    request.insert(QStringLiteral("params"), params);

    QByteArray line = QJsonDocument(request).toJson(QJsonDocument::Compact);
    line.append('\n');
    return line;
}

PythonSidecarResponse PythonSidecarProtocol::parseResponse(const QByteArray &line)
{
    PythonSidecarResponse response;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(line.trimmed(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        response.parseError = parseError.errorString();
        if (response.parseError.isEmpty()) {
            response.parseError = QStringLiteral("Sidecar response must be a JSON object.");
        }
        return response;
    }

    const QJsonObject object = document.object();
    response.valid = true;
    response.id = object.value(QStringLiteral("id")).toString();
    response.ok = object.value(QStringLiteral("ok")).toBool(false);
    if (response.ok) {
        response.result = object.value(QStringLiteral("result")).toObject();
        return response;
    }

    const QJsonObject error = object.value(QStringLiteral("error")).toObject();
    response.error.code = error.value(QStringLiteral("code")).toString();
    response.error.message = error.value(QStringLiteral("message")).toString();
    response.error.retryable = error.value(QStringLiteral("retryable")).toBool(false);
    return response;
}

