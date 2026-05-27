#include "ui/MessageWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStringList>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QVector>
#include <QtGlobal>

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

QString removeTrailingNewline(QString value)
{
    if (value.endsWith(QLatin1Char('\n'))) {
        value.chop(1);
    }

    return value;
}

struct ContentPart {
    bool codeBlock = false;
    QString language;
    QString text;
};

QVector<ContentPart> splitAssistantContent(const QString &content)
{
    QVector<ContentPart> parts;
    QString textBuffer;
    QString codeBuffer;
    QString language;
    bool inCodeBlock = false;

    const QStringList lines = content.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (!inCodeBlock && trimmed.startsWith(QStringLiteral("```"))) {
            const QString text = removeTrailingNewline(textBuffer);
            if (!text.isEmpty()) {
                parts.append(ContentPart{false, QString(), text});
            }

            textBuffer.clear();
            codeBuffer.clear();
            language = trimmed.mid(3).trimmed();
            inCodeBlock = true;
            continue;
        }

        if (inCodeBlock && trimmed == QStringLiteral("```")) {
            parts.append(ContentPart{true, language, removeTrailingNewline(codeBuffer)});
            codeBuffer.clear();
            language.clear();
            inCodeBlock = false;
            continue;
        }

        if (inCodeBlock) {
            codeBuffer += line + QLatin1Char('\n');
        } else {
            textBuffer += line + QLatin1Char('\n');
        }
    }

    if (inCodeBlock) {
        parts.append(ContentPart{true, language, removeTrailingNewline(codeBuffer)});
    } else {
        const QString text = removeTrailingNewline(textBuffer);
        if (!text.isEmpty()) {
            parts.append(ContentPart{false, QString(), text});
        }
    }

    return parts;
}

int codeBlockHeight(const QString &code)
{
    const int lineCount = qMax(1, code.count(QLatin1Char('\n')) + 1);
    return qBound(54, 28 + lineCount * 18, 260);
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

    m_contentLayout = new QVBoxLayout();
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(8);

    headerLayout->addWidget(m_roleLabel);
    headerLayout->addStretch(1);
    headerLayout->addWidget(m_copyButton);

    layout->addLayout(headerLayout);
    layout->addLayout(m_contentLayout);

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
    rebuildContent();
    m_copyButton->setEnabled(!m_content.isEmpty());
}

void MessageWidget::copyContentToClipboard() const
{
    copyTextToClipboard(content());
}

void MessageWidget::copyTextToClipboard(const QString &text) const
{
    QClipboard *clipboard = QApplication::clipboard();
    if (clipboard == nullptr) {
        return;
    }

    clipboard->setText(text);
}

void MessageWidget::rebuildContent()
{
    clearContentWidgets();

    if (m_content.isEmpty()) {
        return;
    }

    if (m_role != MessageRole::Assistant) {
        addTextSegment(m_content);
        return;
    }

    const QVector<ContentPart> parts = splitAssistantContent(m_content);
    for (const ContentPart &part : parts) {
        if (part.codeBlock) {
            addCodeBlock(part.language, part.text);
        } else {
            addTextSegment(part.text);
        }
    }
}

void MessageWidget::clearContentWidgets()
{
    while (m_contentLayout->count() > 0) {
        QLayoutItem *item = m_contentLayout->takeAt(0);
        delete item->widget();
        delete item;
    }
}

void MessageWidget::addTextSegment(const QString &text)
{
    auto *contentLabel = new QLabel(this);
    contentLabel->setObjectName(QStringLiteral("messageContent"));
    contentLabel->setWordWrap(true);
    contentLabel->setTextFormat(m_role == MessageRole::Assistant ? Qt::RichText : Qt::PlainText);
    contentLabel->setOpenExternalLinks(true);
    contentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    contentLabel->setText(m_role == MessageRole::Assistant ? renderAssistantMarkdown(text) : text);
    m_contentLayout->addWidget(contentLabel);
}

void MessageWidget::addCodeBlock(const QString &language, const QString &code)
{
    auto *codeFrame = new QFrame(this);
    codeFrame->setObjectName(QStringLiteral("messageCodeBlockFrame"));

    auto *layout = new QVBoxLayout(codeFrame);
    layout->setContentsMargins(10, 8, 10, 10);
    layout->setSpacing(6);

    auto *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    auto *languageLabel = new QLabel(language.isEmpty() ? QStringLiteral("Code") : language, codeFrame);
    languageLabel->setObjectName(QStringLiteral("messageCodeLanguage"));

    auto *copyCodeButton = new QPushButton(QStringLiteral("Copy code"), codeFrame);
    copyCodeButton->setObjectName(QStringLiteral("copyCodeButton"));
    copyCodeButton->setToolTip(QStringLiteral("Copy code block"));
    copyCodeButton->setFixedHeight(24);
    copyCodeButton->setCursor(Qt::PointingHandCursor);
    copyCodeButton->setEnabled(!code.isEmpty());

    auto *codeEdit = new QPlainTextEdit(codeFrame);
    codeEdit->setObjectName(QStringLiteral("messageCodeBlock"));
    codeEdit->setPlainText(code);
    codeEdit->setReadOnly(true);
    codeEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    codeEdit->setFixedHeight(codeBlockHeight(code));

    headerLayout->addWidget(languageLabel);
    headerLayout->addStretch(1);
    headerLayout->addWidget(copyCodeButton);

    layout->addLayout(headerLayout);
    layout->addWidget(codeEdit);
    m_contentLayout->addWidget(codeFrame);

    connect(copyCodeButton, &QPushButton::clicked, this, [this, code]() {
        copyTextToClipboard(code);
    });
}
