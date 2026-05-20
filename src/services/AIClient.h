#pragma once

#include "core/AppConfig.h"
#include "core/ChatSession.h"

#include <QObject>

class AIClient : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~AIClient() override = default;

    virtual void sendChat(const AppConfig &config, const ChatSession &session) = 0;
    virtual void cancel() = 0;

signals:
    void textDeltaReceived(const QString &delta);
    void requestFinished();
    void requestFailed(const QString &message);
};
