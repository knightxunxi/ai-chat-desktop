#include "ui/MessageWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QLabel>
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

    MessageWidget codeMessage(MessageRole::Assistant, QStringLiteral("```cpp\nint main() { return 0; }\n```"));
    auto *codeContentLabel = codeMessage.findChild<QLabel *>(QStringLiteral("messageContent"));
    assert(codeContentLabel != nullptr);
    assert(codeContentLabel->text().contains(QStringLiteral("<pre")));
    assert(codeContentLabel->text().contains(QStringLiteral("#f1f5f9")));
    assert(codeMessage.content() == QStringLiteral("```cpp\nint main() { return 0; }\n```"));

    message.setContent(QString());
    assert(!copyButton->isEnabled());

    return 0;
}
