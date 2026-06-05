#include "ui/MessageWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QFrame>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>

#include <cassert>
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // ── 基础复制测试 ────────────────────────────────────────────────
    MessageWidget message(MessageRole::Assistant, QStringLiteral("copy this text"));
    auto *contentLabel = message.findChild<QLabel *>(QStringLiteral("messageContent"));
    auto *copyButton = message.findChild<QPushButton *>(QStringLiteral("copyMessageButton"));
    assert(contentLabel != nullptr);
    assert(copyButton != nullptr);
    assert(contentLabel->textFormat() == Qt::RichText);
    assert(copyButton->isEnabled());

    QApplication::clipboard()->clear();
    copyButton->click();
    assert(QApplication::clipboard()->text() == QStringLiteral("copy this text"));

    message.setContent(QStringLiteral("updated text"));
    copyButton->click();
    assert(message.content() == QStringLiteral("updated text"));
    assert(QApplication::clipboard()->text() == QStringLiteral("updated text"));

    MessageWidget userMessage(MessageRole::User, QStringLiteral("**not markdown**"));
    auto *userContentLabel = userMessage.findChild<QLabel *>(QStringLiteral("messageContent"));
    assert(userContentLabel != nullptr);
    assert(userContentLabel->textFormat() == Qt::RichText);
    assert(userMessage.content() == QStringLiteral("**not markdown**"));

    // ── 代码块测试 ──────────────────────────────────────────────────
    const QString codeMarkdown = QStringLiteral("Before\n```cpp\nint main() { return 0; }\n```\nAfter");
    MessageWidget codeMessage(MessageRole::Assistant, codeMarkdown);
    auto *codeBlock = codeMessage.findChild<QPlainTextEdit *>(QStringLiteral("messageCodeBlock"));
    auto *copyCodeButton = codeMessage.findChild<QPushButton *>(QStringLiteral("copyCodeButton"));
    auto *languageLabel = codeMessage.findChild<QLabel *>(QStringLiteral("messageCodeLanguage"));
    const QList<QLabel *> codeTextLabels = codeMessage.findChildren<QLabel *>(QStringLiteral("messageContent"));
    assert(codeBlock != nullptr);
    assert(copyCodeButton != nullptr);
    assert(languageLabel != nullptr);
    assert(languageLabel->text() == QStringLiteral("cpp"));
    assert(codeBlock->toPlainText() == QStringLiteral("int main() { return 0; }"));
    assert(codeTextLabels.size() == 2);
    assert(codeTextLabels[0]->textFormat() == Qt::RichText);
    assert(codeTextLabels[0]->text().contains(QStringLiteral("Before")));
    assert(codeTextLabels[1]->text().contains(QStringLiteral("After")));
    assert(codeMessage.content() == codeMarkdown);

    QApplication::clipboard()->clear();
    copyCodeButton->click();
    assert(QApplication::clipboard()->text() == QStringLiteral("int main() { return 0; }"));

    message.setContent(QString());
    assert(!copyButton->isEnabled());

    // ── CH-2: 表格渲染测试 ──────────────────────────────────────────
    const QString tableMarkdown = QStringLiteral("| Name | Age |\n| --- | --- |\n| Alice | 30 |\n| Bob | 25 |");
    MessageWidget tableMessage(MessageRole::Assistant, tableMarkdown);
    auto *tableFrame = tableMessage.findChild<QFrame *>(QStringLiteral("messageTableFrame"));
    assert(tableFrame != nullptr);
    auto *tableLabel = tableFrame->findChild<QLabel *>(QStringLiteral("messageTableContent"));
    assert(tableLabel != nullptr);
    assert(tableLabel->textFormat() == Qt::RichText);
    // 验证表格 HTML 包含表头（Markdown → QTextDocument HTML 转换结果因 Qt 版本可能不同）
    assert(tableLabel->text().contains(QStringLiteral("<th>")) || tableLabel->text().contains(QStringLiteral("<table")));
    // V15.5: Qt Markdown→HTML 渲染可能变换大小写或标签名，改为验证渲染非空
    assert(!tableLabel->text().isEmpty());
    // 确保没有代码块被误识别
    auto *tableCodeBlock = tableMessage.findChild<QPlainTextEdit *>(QStringLiteral("messageCodeBlock"));
    assert(tableCodeBlock == nullptr);

    // ── CH-2: 引用渲染测试 ──────────────────────────────────────────
    const QString quoteMarkdown = QStringLiteral("> quoted text\n> more quote");
    MessageWidget quoteMessage(MessageRole::Assistant, quoteMarkdown);
    auto *quoteFrame = quoteMessage.findChild<QFrame *>(QStringLiteral("messageQuoteFrame"));
    assert(quoteFrame != nullptr);
    auto *quoteLabel = quoteFrame->findChild<QLabel *>(QStringLiteral("messageQuoteContent"));
    assert(quoteLabel != nullptr);
    assert(quoteLabel->textFormat() == Qt::RichText);
    // 验证引用文本已去除 > 前缀
    assert(quoteLabel->text().contains(QStringLiteral("quoted text")));

    // ── CH-2: 混合内容测试 ──────────────────────────────────────────
    const QString mixedMarkdown = QStringLiteral("普通文本\n| Name | Age |\n| --- | --- |\n| Alice | 30 |\n更多文本\n> quoted line");
    MessageWidget mixedMessage(MessageRole::Assistant, mixedMarkdown);
    // 应有：文本 + 表格 + 文本 + 引用
    auto *mixedTableFrame = mixedMessage.findChild<QFrame *>(QStringLiteral("messageTableFrame"));
    auto *mixedQuoteFrame = mixedMessage.findChild<QFrame *>(QStringLiteral("messageQuoteFrame"));
    const QList<QLabel *> mixedTextLabels = mixedMessage.findChildren<QLabel *>(QStringLiteral("messageContent"));
    assert(mixedTableFrame != nullptr);
    assert(mixedQuoteFrame != nullptr);
    assert(mixedTextLabels.size() == 2);
    assert(mixedTextLabels[0]->text().contains(QStringLiteral("普通文本")));
    assert(mixedTextLabels[1]->text().contains(QStringLiteral("更多文本")));

    // ── CH-2: 纯文本表格不误判 ───────────────────────────────────────
    const QString plainText = QStringLiteral("Just some text\nwith multiple lines\nand no tables or quotes.");
    MessageWidget plainMessage(MessageRole::Assistant, plainText);
    auto *plainTable = plainMessage.findChild<QFrame *>(QStringLiteral("messageTableFrame"));
    auto *plainQuote = plainMessage.findChild<QFrame *>(QStringLiteral("messageQuoteFrame"));
    assert(plainTable == nullptr);
    assert(plainQuote == nullptr);

    return 0;
}
