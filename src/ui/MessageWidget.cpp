#include "ui/MessageWidget.h"

#include <QLabel>
#include <QVBoxLayout>

namespace {

QString roleLabel(MessageRole role)
{
    switch (role) {
    case MessageRole::System:
        return QStringLiteral("System");
    case MessageRole::User:
        return QStringLiteral("You");
    case MessageRole::Assistant:
        return QStringLiteral("AI");
    }

    return QStringLiteral("Message");
}

QString roleObjectName(MessageRole role)
{
    switch (role) {
    case MessageRole::System:
        return QStringLiteral("systemMessage");
    case MessageRole::User:
        return QStringLiteral("userMessage");
    case MessageRole::Assistant:
        return QStringLiteral("assistantMessage");
    }

    return QStringLiteral("message");
}

} // namespace

MessageWidget::MessageWidget(MessageRole role, const QString &content, QWidget *parent)
    : QFrame(parent)
    , m_role(role)
{
    setObjectName(roleObjectName(role));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 9, 14, 9);
    layout->setSpacing(4);

    m_roleLabel = new QLabel(roleLabel(role), this);
    m_roleLabel->setObjectName(QStringLiteral("messageRole"));

    m_contentLabel = new QLabel(this);
    m_contentLabel->setObjectName(QStringLiteral("messageContent"));
    m_contentLabel->setWordWrap(true);
    m_contentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    layout->addWidget(m_roleLabel);
    layout->addWidget(m_contentLabel);

    setContent(content);
}

MessageRole MessageWidget::role() const
{
    return m_role;
}

QString MessageWidget::content() const
{
    return m_contentLabel->text();
}

void MessageWidget::setContent(const QString &content)
{
    m_contentLabel->setText(content);
}
