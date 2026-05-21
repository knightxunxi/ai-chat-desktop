#include "ui/MessageWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QPushButton>

#include <cassert>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MessageWidget message(MessageRole::Assistant, QStringLiteral("copy this text"));
    auto *copyButton = message.findChild<QPushButton *>(QStringLiteral("copyMessageButton"));
    assert(copyButton != nullptr);
    assert(copyButton->isEnabled());

    QApplication::clipboard()->clear();
    copyButton->click();
    assert(QApplication::clipboard()->text() == QStringLiteral("copy this text"));

    message.setContent(QStringLiteral("updated text"));
    copyButton->click();
    assert(message.content() == QStringLiteral("updated text"));
    assert(QApplication::clipboard()->text() == QStringLiteral("updated text"));

    message.setContent(QString());
    assert(!copyButton->isEnabled());

    return 0;
}
