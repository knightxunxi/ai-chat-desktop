#include "ui/MessageWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>

#include <cassert>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

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
    assert(userContentLabel->textFormat() == Qt::PlainText);
    assert(userMessage.content() == QStringLiteral("**not markdown**"));

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

    return 0;
}
