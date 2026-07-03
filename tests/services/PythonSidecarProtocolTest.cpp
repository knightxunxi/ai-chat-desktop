#include "services/PythonSidecarProtocol.h"

#include <QJsonDocument>
#include <QJsonObject>

#include <cassert>

int main()
{
    const QByteArray request = PythonSidecarProtocol::buildRequest(
        QStringLiteral("req-1"),
        QStringLiteral("token.count"),
        QJsonObject{{QStringLiteral("text"), QStringLiteral("hello")}});
    assert(request.endsWith('\n'));

    const QJsonDocument requestDocument = QJsonDocument::fromJson(request);
    assert(requestDocument.isObject());
    const QJsonObject requestObject = requestDocument.object();
    assert(requestObject.value(QStringLiteral("id")).toString() == QStringLiteral("req-1"));
    assert(requestObject.value(QStringLiteral("method")).toString() == QStringLiteral("token.count"));
    assert(requestObject.value(QStringLiteral("params")).toObject().value(QStringLiteral("text")).toString() == QStringLiteral("hello"));

    const PythonSidecarResponse success = PythonSidecarProtocol::parseResponse(
        QByteArrayLiteral(R"({"id":"req-1","ok":true,"result":{"tokens":12}})"));
    assert(success.valid);
    assert(success.ok);
    assert(success.id == QStringLiteral("req-1"));
    assert(success.result.value(QStringLiteral("tokens")).toInt() == 12);

    const PythonSidecarResponse failure = PythonSidecarProtocol::parseResponse(
        QByteArrayLiteral(R"({"id":"req-2","ok":false,"error":{"code":"provider_error","message":"failed","retryable":true}})"));
    assert(failure.valid);
    assert(!failure.ok);
    assert(failure.id == QStringLiteral("req-2"));
    assert(failure.error.code == QStringLiteral("provider_error"));
    assert(failure.error.message == QStringLiteral("failed"));
    assert(failure.error.retryable);

    const PythonSidecarResponse invalid = PythonSidecarProtocol::parseResponse(QByteArrayLiteral("{"));
    assert(!invalid.valid);
    assert(!invalid.parseError.isEmpty());

    return 0;
}

