#include "ui/MessageWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextDocument>
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

QString markdownStyleSheet()
{
    return QStringLiteral(
        "<style>"
        "body { color: #1f2937; font-family: 'Segoe UI', 'Microsoft YaHei UI', sans-serif; font-size: 14px; }"
        "p { margin-top: 0; margin-bottom: 8px; }"
        "ul, ol { margin-top: 4px; margin-bottom: 8px; }"
        "pre { background-color: #f1f5f9; border: 1px solid #cbd5e1; border-radius: 6px; color: #0f172a; font-family: Consolas, 'Cascadia Mono', monospace; margin-top: 8px; margin-bottom: 8px; padding: 8px 10px; white-space: pre-wrap; }"
        "code { background-color: #eef2f7; color: #0f172a; font-family: Consolas, 'Cascadia Mono', monospace; }"
        "pre code { background-color: transparent; }"
        "</style>");
}

QString renderAssistantMarkdown(const QString &content)
{
    QTextDocument document;
    document.setMarkdown(content, QTextDocument::MarkdownDialectGitHub);

    QString html = document.toHtml();
    html.replace(QStringLiteral("</head>"), markdownStyleSheet() + QStringLiteral("</head>"));
    return html;
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

    auto *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    m_roleLabel = new QLabel(roleLabel(role), this);
    m_roleLabel->setObjectName(QStringLiteral("messageRole"));

    m_copyButton = new QPushButton(QStringLiteral("Copy"), this);
    m_copyButton->setObjectName(QStringLiteral("copyMessageButton"));
    m_copyButton->setToolTip(QStringLiteral("Copy message"));
    m_copyButton->setFixedHeight(24);
    m_copyButton->setCursor(Qt::PointingHandCursor);

    m_contentLabel = new QLabel(this);
    m_contentLabel->setObjectName(QStringLiteral("messageContent"));
    m_contentLabel->setWordWrap(true);
    m_contentLabel->setTextFormat(m_role == MessageRole::Assistant ? Qt::RichText : Qt::PlainText);
    m_contentLabel->setOpenExternalLinks(true);
    m_contentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);

    headerLayout->addWidget(m_roleLabel);
    headerLayout->addStretch(1);
    headerLayout->addWidget(m_copyButton);

    layout->addLayout(headerLayout);
    layout->addWidget(m_contentLabel);

    connect(m_copyButton, &QPushButton::clicked, this, &MessageWidget::copyContentToClipboard);

    setContent(content);
}

MessageRole MessageWidget::role() const
{
    return m_role;
}

QString MessageWidget::content() const
{
    return m_content;
}

void MessageWidget::setContent(const QString &content)
{
    m_content = content;
    m_contentLabel->setText(m_role == MessageRole::Assistant ? renderAssistantMarkdown(m_content) : m_content);
    m_copyButton->setEnabled(!m_content.isEmpty());
}

void MessageWidget::copyContentToClipboard() const
{
    QClipboard *clipboard = QApplication::clipboard();
    if (clipboard == nullptr) {
        return;
    }

    clipboard->setText(content());
}
