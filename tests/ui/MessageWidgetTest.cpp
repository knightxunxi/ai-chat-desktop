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
    assert(contentLabel->textFormat() == Qt::MarkdownText);
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

    message.setContent(QString());
    assert(!copyButton->isEnabled());

    return 0;
}
