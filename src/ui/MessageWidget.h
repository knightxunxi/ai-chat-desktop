#pragma once

#include "core/MessageRole.h"

#include <QFrame>

class QLabel;
class QPushButton;
class QVBoxLayout;

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
    void copyTextToClipboard(const QString &text) const;
    void rebuildContent();
    void clearContentWidgets();
    void addTextSegment(const QString &text);
    void addCodeBlock(const QString &language, const QString &code);

    MessageRole m_role = MessageRole::User;
    QString m_content;
    QLabel *m_roleLabel = nullptr;
    QVBoxLayout *m_contentLayout = nullptr;
    QPushButton *m_copyButton = nullptr;
};
