#include "ui/ToolsDialog.h"

#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>

#include <cassert>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ToolsDialog dialog(AppLanguage::Chinese);

    auto *toolComboBox = dialog.findChild<QComboBox *>(QStringLiteral("toolComboBox"));
    auto *descriptionLabel = dialog.findChild<QLabel *>(QStringLiteral("toolDescriptionLabel"));
    auto *inputEdit = dialog.findChild<QPlainTextEdit *>(QStringLiteral("toolInputEdit"));
    auto *outputEdit = dialog.findChild<QPlainTextEdit *>(QStringLiteral("toolOutputEdit"));
    auto *runButton = dialog.findChild<QPushButton *>(QStringLiteral("runToolButton"));
    auto *copyButton = dialog.findChild<QPushButton *>(QStringLiteral("copyToolOutputButton"));
    auto *insertButton = dialog.findChild<QPushButton *>(QStringLiteral("insertToolOutputButton"));
    auto *closeButton = dialog.findChild<QPushButton *>(QStringLiteral("closeToolsButton"));

    assert(toolComboBox != nullptr);
    assert(descriptionLabel != nullptr);
    assert(inputEdit != nullptr);
    assert(outputEdit != nullptr);
    assert(runButton != nullptr);
    assert(copyButton != nullptr);
    assert(insertButton != nullptr);
    assert(closeButton != nullptr);
    assert(toolComboBox->count() >= 4);
    assert(!descriptionLabel->text().isEmpty());
    assert(!copyButton->isEnabled());
    assert(!insertButton->isEnabled());

    inputEdit->setPlainText(QStringLiteral("{\"name\":\"test\",\"items\":[1,2]}"));
    runButton->click();
    assert(outputEdit->toPlainText().contains('\n'));
    assert(outputEdit->toPlainText().contains(QStringLiteral("\"name\": \"test\"")));
    assert(copyButton->isEnabled());
    assert(insertButton->isEnabled());

    QString insertedOutput;
    QObject::connect(&dialog, &ToolsDialog::outputInsertionRequested, [&insertedOutput](const QString &output) {
        insertedOutput = output;
    });
    insertButton->click();
    assert(insertedOutput == outputEdit->toPlainText());

    const int compactIndex = toolComboBox->findData(QStringLiteral("json.compact"));
    assert(compactIndex >= 0);
    toolComboBox->setCurrentIndex(compactIndex);
    inputEdit->setPlainText(QStringLiteral("{\n  \"name\": \"test\",\n  \"items\": [1, 2]\n}"));
    runButton->click();
    assert(!outputEdit->toPlainText().contains('\n'));

    inputEdit->setPlainText(QStringLiteral("{\"name\":"));
    runButton->click();
    assert(outputEdit->toPlainText().isEmpty());
    assert(!copyButton->isEnabled());
    assert(!insertButton->isEnabled());

    closeButton->click();
    assert(dialog.result() == QDialog::Accepted);

    return 0;
}
