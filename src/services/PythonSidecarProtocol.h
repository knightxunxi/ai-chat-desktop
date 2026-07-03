#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>

struct PythonSidecarError {
    QString code;
    QString message;
    bool retryable = false;
};

struct PythonSidecarResponse {
    bool valid = false;
    QString parseError;
    QString id;
    bool ok = false;
    QJsonObject result;
    PythonSidecarError error;
};

class PythonSidecarProtocol
{
public:
    static QByteArray buildRequest(const QString &id, const QString &method, const QJsonObject &params = QJsonObject());
    static PythonSidecarResponse parseResponse(const QByteArray &line);
};

