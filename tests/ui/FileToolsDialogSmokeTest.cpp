#include "ui/FileToolsDialog.h"

#include <QApplication>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>

#include <cassert>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    FileToolsDialog dialog(AppLanguage::Chinese);

    auto *titleLabel = dialog.findChild<QLabel *>(QStringLiteral("fileToolsTitleLabel"));
    auto *descriptionLabel = dialog.findChild<QLabel *>(QStringLiteral("fileToolsDescriptionLabel"));
    auto *statusLabel = dialog.findChild<QLabel *>(QStringLiteral("fileToolStatusLabel"));
    auto *outputEdit = dialog.findChild<QPlainTextEdit *>(QStringLiteral("fileToolOutputEdit"));
    auto *readFileButton = dialog.findChild<QPushButton *>(QStringLiteral("readFileButton"));
    auto *listDirectoryButton = dialog.findChild<QPushButton *>(QStringLiteral("listDirectoryButton"));
    auto *saveOutputButton = dialog.findChild<QPushButton *>(QStringLiteral("saveFileOutputButton"));
    auto *openFileButton = dialog.findChild<QPushButton *>(QStringLiteral("openFileButton"));
    auto *openDirectoryButton = dialog.findChild<QPushButton *>(QStringLiteral("openDirectoryButton"));
    auto *copyButton = dialog.findChild<QPushButton *>(QStringLiteral("copyFileToolOutputButton"));
    auto *insertButton = dialog.findChild<QPushButton *>(QStringLiteral("insertFileToolOutputButton"));
    auto *closeButton = dialog.findChild<QPushButton *>(QStringLiteral("closeFileToolsButton"));

    assert(titleLabel != nullptr);
    assert(descriptionLabel != nullptr);
    assert(statusLabel != nullptr);
    assert(outputEdit != nullptr);
    assert(readFileButton != nullptr);
    assert(listDirectoryButton != nullptr);
    assert(saveOutputButton != nullptr);
    assert(openFileButton != nullptr);
    assert(openDirectoryButton != nullptr);
    assert(copyButton != nullptr);
    assert(insertButton != nullptr);
    assert(closeButton != nullptr);

    assert(!titleLabel->text().isEmpty());
    assert(!descriptionLabel->text().isEmpty());
    assert(!statusLabel->text().isEmpty());
    assert(outputEdit->isReadOnly());
    assert(readFileButton->isEnabled());
    assert(listDirectoryButton->isEnabled());
    assert(openFileButton->isEnabled());
    assert(openDirectoryButton->isEnabled());
    assert(!saveOutputButton->isEnabled());
    assert(!copyButton->isEnabled());
    assert(!insertButton->isEnabled());

    closeButton->click();
    assert(dialog.result() == QDialog::Accepted);

    return 0;
}
