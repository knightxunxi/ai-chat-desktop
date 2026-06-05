#include "ui/MessageWidget.h"
#include "ui/CodeHighlighter.h"

#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
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

// V16.3: 检测当前是否为暗色模式
bool isDarkMode()
{
    return qApp->property("darkMode").toBool();
}

QString markdownStyleSheet()
{
    if (isDarkMode()) {
        return QStringLiteral(
            "<style>"
            "body { color: #e5e7eb; font-family: 'Segoe UI', 'Microsoft YaHei UI', sans-serif; font-size: 14px; }"
            "p { margin-top: 0; margin-bottom: 8px; }"
            "ul, ol { margin-top: 4px; margin-bottom: 8px; }"
            "pre { background-color: #1e293b; border: 1px solid #334155; border-radius: 6px; color: #e2e8f0; font-family: Consolas, 'Cascadia Mono', monospace; margin-top: 8px; margin-bottom: 8px; padding: 8px 10px; white-space: pre-wrap; }"
            "code { background-color: #334155; color: #e2e8f0; font-family: Consolas, 'Cascadia Mono', monospace; }"
            "pre code { background-color: transparent; }"
            "table { border-collapse: collapse; width: 100%; }"
            "th { background-color: #334155; color: #e2e8f0; padding: 6px 12px; text-align: left; border: 1px solid #475569; }"
            "td { padding: 6px 12px; border: 1px solid #475569; color: #e2e8f0; }"
            "</style>");
    }

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

// ─── ContentPart ────────────────────────────────────────────────────
struct ContentPart {
    bool codeBlock  = false;
    bool tableBlock = false;
    bool quoteBlock = false;
    QString language;
    QString text;
};

// ─── 行分类辅助 ──────────────────────────────────────────────────────
enum class LineKind { Text, Table, Quote };

LineKind classifyLine(const QString &line)
{
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty()) {
        return LineKind::Text;
    }
    // 表格行：以 | 开头（且不是引用 `> |` 的情况）
    if (trimmed.startsWith(QLatin1Char('|'))) {
        return LineKind::Table;
    }
    // 引用行：以 >  或 > 开头
    if (trimmed.startsWith(QStringLiteral("> ")) || trimmed == QStringLiteral(">")) {
        return LineKind::Quote;
    }
    return LineKind::Text;
}

/// 功能：对非代码块 ContentPart 做二次扫描，分离表格/引用段落。
QVector<ContentPart> splitTableAndQuoteParts(const QVector<ContentPart> &parts)
{
    QVector<ContentPart> result;

    for (const ContentPart &part : parts) {
        if (part.codeBlock) {
            result.append(part);
            continue;
        }

        // 对普通文本部分逐行扫描
        const QStringList lines = part.text.split(QLatin1Char('\n'), Qt::KeepEmptyParts);

        QString buffer;
        LineKind bufferKind = LineKind::Text;

        auto flush = [&]() {
            if (buffer.isEmpty()) {
                return;
            }
            ContentPart cp;
            cp.text = removeTrailingNewline(buffer);
            if (bufferKind == LineKind::Table) {
                cp.tableBlock = true;
            } else if (bufferKind == LineKind::Quote) {
                cp.quoteBlock = true;
            }
            result.append(cp);
            buffer.clear();
        };

        for (const QString &line : lines) {
            const LineKind kind = classifyLine(line);

            if (kind == LineKind::Text) {
                if (bufferKind == LineKind::Table) {
                    // 表格段结束
                    flush();
                    bufferKind = LineKind::Text;
                } else if (bufferKind == LineKind::Quote) {
                    // 引用段结束
                    flush();
                    bufferKind = LineKind::Text;
                }
            } else if (kind == LineKind::Table) {
                if (bufferKind == LineKind::Quote) {
                    flush();
                }
                if (bufferKind != LineKind::Table) {
                    flush();
                    bufferKind = LineKind::Table;
                }
            } else if (kind == LineKind::Quote) {
                if (bufferKind == LineKind::Table) {
                    flush();
                }
                if (bufferKind != LineKind::Quote) {
                    flush();
                    bufferKind = LineKind::Quote;
                }
            }

            buffer += line + QLatin1Char('\n');
        }

        flush();
    }

    return result;
}

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
                parts.append(ContentPart{false, false, false, QString(), text});
            }

            textBuffer.clear();
            codeBuffer.clear();
            language = trimmed.mid(3).trimmed();
            inCodeBlock = true;
            continue;
        }

        if (inCodeBlock && trimmed == QStringLiteral("```")) {
            parts.append(ContentPart{true, false, false, language, removeTrailingNewline(codeBuffer)});
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
        parts.append(ContentPart{true, false, false, language, removeTrailingNewline(codeBuffer)});
    } else {
        const QString text = removeTrailingNewline(textBuffer);
        if (!text.isEmpty()) {
            parts.append(ContentPart{false, false, false, QString(), text});
        }
    }

    // CH-2: 二次扫描，分离表格和引用段落
    return splitTableAndQuoteParts(parts);
}

int codeBlockHeight(const QString &code)
{
    const int lineCount = qMax(1, code.count(QLatin1Char('\n')) + 1);
    return qBound(54, 28 + lineCount * 18, 260);
}

} // namespace

// ─── MessageWidget::MessageWidget ────────────────────────────────────
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

void MessageWidget::updateContentIncremental(const QString &content)
{
    m_content = content;
    m_copyButton->setEnabled(!m_content.isEmpty());

    // 如果只有 1 个文本段（有 messageContent objectName 的 QLabel），直接 setText
    if (m_contentLayout->count() == 1) {
        auto *lastWidget = m_contentLayout->itemAt(0)->widget();
        if (lastWidget != nullptr && lastWidget->objectName() == QStringLiteral("messageContent")) {
            auto *label = qobject_cast<QLabel *>(lastWidget);
            if (label != nullptr) {
                label->setText(renderAssistantMarkdown(content));
                return;
            }
        }
    }

    // 如果最后一个控件是代码块容器（messageCodeBlockFrame）
    if (m_contentLayout->count() > 0) {
        auto *lastItem = m_contentLayout->itemAt(m_contentLayout->count() - 1);
        auto *lastWidget = lastItem->widget();
        if (lastWidget != nullptr && lastWidget->objectName() == QStringLiteral("messageCodeBlockFrame")) {
            // 查找代码块内的 QPlainTextEdit 并更新
            auto *codeEdit = lastWidget->findChild<QPlainTextEdit *>(QStringLiteral("messageCodeBlock"));
            if (codeEdit != nullptr) {
                // 从 content 中提取最后一个代码块内容
                const QVector<ContentPart> parts = splitAssistantContent(content);
                if (!parts.isEmpty() && parts.last().codeBlock) {
                    codeEdit->setPlainText(parts.last().text);
                    codeEdit->setFixedHeight(codeBlockHeight(parts.last().text));
                    return;
                }
            }
        }

        // CH-2: 最后一个控件是表格容器
        if (lastWidget != nullptr && lastWidget->objectName() == QStringLiteral("messageTableFrame")) {
            const QVector<ContentPart> parts = splitAssistantContent(content);
            if (!parts.isEmpty() && parts.last().tableBlock) {
                // 表格内容变化时完整重建
                rebuildContent();
                return;
            }
        }

        // CH-2: 最后一个控件是引用容器
        if (lastWidget != nullptr && lastWidget->objectName() == QStringLiteral("messageQuoteFrame")) {
            const QVector<ContentPart> parts = splitAssistantContent(content);
            if (!parts.isEmpty() && parts.last().quoteBlock) {
                rebuildContent();
                return;
            }
        }
    }

    // 兜底：完整重建
    rebuildContent();
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
        } else if (part.tableBlock) {
            addTableBlock(part.text);
        } else if (part.quoteBlock) {
            addQuoteBlock(part.text);
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
    // CH-5: User 消息也支持 Markdown 渲染
    contentLabel->setTextFormat(Qt::RichText);
    contentLabel->setOpenExternalLinks(true);
    contentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    contentLabel->setText(renderAssistantMarkdown(text));
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

    // CH-3: 挂接语法高亮器
    auto *highlighter = new CodeHighlighter(language, codeEdit->document());
    Q_UNUSED(highlighter); // 由 document 管理生命周期

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

// ─── CH-2: addTableBlock ────────────────────────────────────────────
void MessageWidget::addTableBlock(const QString &markdownTable)
{
    auto *tableFrame = new QFrame(this);
    tableFrame->setObjectName(QStringLiteral("messageTableFrame"));

    auto *layout = new QVBoxLayout(tableFrame);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 将 Markdown 表格转换为 HTML <table>
    const QStringList rows = markdownTable.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    // 收集有效的数据行（跳过纯分隔行，如 | --- | --- |）
    QStringList dataRows;
    for (const QString &row : rows) {
        const QString trimmed = row.trimmed();
        // 分隔行特征：包含 | - | 模式的（至少包含 --- 的）
        if (trimmed.contains(QRegularExpression(QStringLiteral("\\|\\s*-{3,}\\s*\\|")))) {
            continue;
        }
        if (!trimmed.isEmpty()) {
            dataRows.append(trimmed);
        }
    }

    if (dataRows.isEmpty()) {
        m_contentLayout->addWidget(tableFrame);
        return;
    }

    auto parseCells = [](const QString &row) -> QStringList {
        QStringList cells;
        QString text = row.trimmed();
        // 去掉首尾的 |
        if (text.startsWith(QLatin1Char('|'))) {
            text = text.mid(1);
        }
        if (text.endsWith(QLatin1Char('|'))) {
            text.chop(1);
        }
        const QStringList parts = text.split(QLatin1Char('|'));
        for (const QString &part : parts) {
            cells.append(part.trimmed());
        }
        return cells;
    };

    QString html = QStringLiteral("<table>");

    // 第一行为表头
    if (!dataRows.isEmpty()) {
        html += QStringLiteral("<thead><tr>");
        const QStringList headerCells = parseCells(dataRows.first());
        for (const QString &cell : headerCells) {
            // 内联 Markdown -> HTML（支持加粗等）
            QTextDocument doc;
            doc.setMarkdown(cell);
            html += QStringLiteral("<th>") + doc.toHtml().section(QStringLiteral("<body>"), 1).section(QStringLiteral("</body>"), 0, 0) + QStringLiteral("</th>");
        }
        html += QStringLiteral("</tr></thead>");
    }

    // 其余行为表体
    if (dataRows.size() > 1) {
        html += QStringLiteral("<tbody>");
        for (int i = 1; i < dataRows.size(); ++i) {
            html += QStringLiteral("<tr>");
            const QStringList cells = parseCells(dataRows.at(i));
            for (const QString &cell : cells) {
                QTextDocument doc;
                doc.setMarkdown(cell);
                html += QStringLiteral("<td>") + doc.toHtml().section(QStringLiteral("<body>"), 1).section(QStringLiteral("</body>"), 0, 0) + QStringLiteral("</td>");
            }
            html += QStringLiteral("</tr>");
        }
        html += QStringLiteral("</tbody>");
    }

    html += QStringLiteral("</table>");

    auto *tableLabel = new QLabel(tableFrame);
    tableLabel->setObjectName(QStringLiteral("messageTableContent"));
    tableLabel->setTextFormat(Qt::RichText);
    tableLabel->setWordWrap(true);
    tableLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    tableLabel->setText(html);

    layout->addWidget(tableLabel);
    m_contentLayout->addWidget(tableFrame);
}

// ─── CH-2: addQuoteBlock ────────────────────────────────────────────
void MessageWidget::addQuoteBlock(const QString &quoteText)
{
    auto *quoteFrame = new QFrame(this);
    quoteFrame->setObjectName(QStringLiteral("messageQuoteFrame"));

    auto *layout = new QVBoxLayout(quoteFrame);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 去掉每行的 `> ` 前缀
    QString cleaned;
    const QStringList lines = quoteText.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("> "))) {
            trimmed = trimmed.mid(2);
        } else if (trimmed == QStringLiteral(">")) {
            trimmed.clear();
        }
        cleaned += trimmed + QLatin1Char('\n');
    }
    cleaned = removeTrailingNewline(cleaned);

    auto *quoteLabel = new QLabel(quoteFrame);
    quoteLabel->setObjectName(QStringLiteral("messageQuoteContent"));
    quoteLabel->setWordWrap(true);
    quoteLabel->setTextFormat(Qt::RichText);
    quoteLabel->setOpenExternalLinks(true);
    quoteLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    quoteLabel->setText(renderAssistantMarkdown(cleaned));

    layout->addWidget(quoteLabel);
    m_contentLayout->addWidget(quoteFrame);
}

// ─── V16.3: contextMenuEvent ────────────────────────────────────────
void MessageWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    QAction *copyAction = menu.addAction(tr("Copy Message", "复制消息"));
    connect(copyAction, &QAction::triggered, this, &MessageWidget::copyContentToClipboard);

    menu.addSeparator();

    // CH-8: 编辑选项（仅 User 消息可见）
    if (m_role == MessageRole::User) {
        QAction *editAction = menu.addAction(tr("Edit Message", "编辑消息"));
        connect(editAction, &QAction::triggered, this, &MessageWidget::enterEditMode);
    }

    QAction *quoteAction = menu.addAction(tr("Quote Reply", "引用回复"));
    connect(quoteAction, &QAction::triggered, this, [this]() {
        emit quoteReplyRequested(m_content);
    });

    QAction *regenerateAction = menu.addAction(tr("Regenerate", "重新生成"));
    regenerateAction->setEnabled(m_role == MessageRole::Assistant);
    connect(regenerateAction, &QAction::triggered, this, [this]() {
        emit regenerateRequested();
    });

    menu.addSeparator();

    QAction *deleteAction = menu.addAction(tr("Delete Message", "删除消息"));
    connect(deleteAction, &QAction::triggered, this, [this]() {
        emit deleteRequested();
    });

    menu.exec(event->globalPos());
}

// ─── CH-8: mouseDoubleClickEvent ─────────────────────────────────────
void MessageWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (m_role == MessageRole::User) {
        enterEditMode();
        return;
    }
    QFrame::mouseDoubleClickEvent(event);
}

// ─── CH-8: enterEditMode ─────────────────────────────────────────────
void MessageWidget::enterEditMode()
{
    // 只能编辑 User 消息
    if (m_role != MessageRole::User) {
        return;
    }

    // 如果已经在编辑模式，忽略二次触发
    if (m_editWidget != nullptr) {
        return;
    }

    // 找到 messageContent 的 QLabel，隐藏它
    for (int i = 0; i < m_contentLayout->count(); ++i) {
        auto *item = m_contentLayout->itemAt(i);
        if (item == nullptr) {
            continue;
        }
        auto *w = item->widget();
        if (w != nullptr && w->objectName() == QStringLiteral("messageContent")) {
            w->setVisible(false);
            break;
        }
    }

    // 创建编辑框
    m_editWidget = new QPlainTextEdit(this);
    m_editWidget->setObjectName(QStringLiteral("messageEditInput"));
    m_editWidget->setPlainText(m_content);
    m_editWidget->setMinimumHeight(60);
    m_editWidget->setMaximumHeight(120);

    // 按钮行
    auto *btnFrame = new QFrame(this);
    btnFrame->setObjectName(QStringLiteral("editButtonBar"));
    auto *btnLayout = new QHBoxLayout(btnFrame);
    btnLayout->setContentsMargins(0, 4, 0, 0);
    btnLayout->setSpacing(8);

    auto *confirmBtn = new QPushButton(tr("Confirm", "确认"), btnFrame);
    confirmBtn->setObjectName(QStringLiteral("editConfirmButton"));
    auto *cancelBtn = new QPushButton(tr("Cancel", "取消"), btnFrame);
    cancelBtn->setObjectName(QStringLiteral("editCancelButton"));

    btnLayout->addStretch();
    btnLayout->addWidget(confirmBtn);
    btnLayout->addWidget(cancelBtn);

    m_contentLayout->addWidget(m_editWidget);
    m_contentLayout->addWidget(btnFrame);

    connect(confirmBtn, &QPushButton::clicked, this, [this]() {
        QString newContent = m_editWidget->toPlainText().trimmed();
        if (!newContent.isEmpty()) {
            m_content = newContent;
            emit editConfirmed(newContent);
        }
        exitEditMode();
    });
    connect(cancelBtn, &QPushButton::clicked, this, [this]() {
        exitEditMode();
        emit editCancelled();
    });

    m_editWidget->setFocus();
}

// ─── CH-8: exitEditMode ──────────────────────────────────────────────
void MessageWidget::exitEditMode()
{
    if (m_editWidget != nullptr) {
        m_contentLayout->removeWidget(m_editWidget);
        m_editWidget->deleteLater();
        m_editWidget = nullptr;
    }

    // 删除按钮栏
    auto *btnBar = findChild<QFrame *>(QStringLiteral("editButtonBar"));
    if (btnBar != nullptr) {
        m_contentLayout->removeWidget(btnBar);
        btnBar->deleteLater();
    }

    // 恢复显示原 QLabel
    for (int i = 0; i < m_contentLayout->count(); ++i) {
        auto *item = m_contentLayout->itemAt(i);
        if (item == nullptr) {
            continue;
        }
        auto *w = item->widget();
        if (w != nullptr && w->objectName() == QStringLiteral("messageContent")) {
            w->setVisible(true);
            break;
        }
    }
}
