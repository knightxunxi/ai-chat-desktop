#pragma once

#include "core/MessageRole.h"

#include <QFrame>

class QLabel;
class QPushButton;

class MessageWidget : public QFrame
{
    Q_OBJECT

public:
    explicit MessageWidget(MessageRole role, const QString &content, QWidget *parent = nullptr);

    MessageRole role() const;
    QString content() const;
    void setContent(const QString &content);

private:
    void copyContentToClipboard() const;

    MessageRole m_role = MessageRole::User;
    QString m_content;
    QLabel *m_roleLabel = nullptr;
    QLabel *m_contentLabel = nullptr;
    QPushButton *m_copyButton = nullptr;
};
