#pragma once

#include "core/MessageRole.h"

#include <QFrame>

class QLabel;

class MessageWidget : public QFrame
{
    Q_OBJECT

public:
    explicit MessageWidget(MessageRole role, const QString &content, QWidget *parent = nullptr);

    MessageRole role() const;
    QString content() const;
    void setContent(const QString &content);

private:
    MessageRole m_role = MessageRole::User;
    QLabel *m_roleLabel = nullptr;
    QLabel *m_contentLabel = nullptr;
};
