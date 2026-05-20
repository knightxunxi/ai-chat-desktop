#pragma once

#include "services/AIClient.h"
#include "services/StreamParser.h"

#include <QNetworkAccessManager>
#include <QPointer>
#include <QUrl>

class QNetworkReply;

class OpenAICompatibleClient : public AIClient
{
    Q_OBJECT

public:
    explicit OpenAICompatibleClient(QObject *parent = nullptr);

    void sendChat(const AppConfig &config, const ChatSession &session) override;
    void cancel() override;

private:
    QUrl chatCompletionsUrl(const QString &baseUrl) const;
    QByteArray buildRequestBody(const AppConfig &config, const ChatSession &session) const;
    QString extractErrorMessage(const QByteArray &body, const QString &fallback) const;
    void handleReadyRead();
    void handleFinished();

    QNetworkAccessManager m_networkManager;
    QPointer<QNetworkReply> m_currentReply;
    StreamParser m_streamParser;
    QByteArray m_errorBody;
    bool m_doneReceived = false;
};
