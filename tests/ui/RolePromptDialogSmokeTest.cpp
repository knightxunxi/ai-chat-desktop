#include "ui/RolePromptDialog.h"

#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>

#include <cassert>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QVector<PromptTemplate> templates;
    templates.append(PromptTemplate{
        QStringLiteral("test-template"),
        QStringLiteral("Test Template"),
        QStringLiteral("You are a test assistant.")
    });

    RolePromptDialog dialog(QStringLiteral("Initial prompt"), templates, AppLanguage::English);
    auto *combo = dialog.findChild<QComboBox *>(QStringLiteral("roleTemplateCombo"));
    auto *nameEdit = dialog.findChild<QLineEdit *>(QStringLiteral("roleTemplateNameEdit"));
    auto *promptEdit = dialog.findChild<QTextEdit *>(QStringLiteral("rolePromptEdit"));
    auto *importButton = dialog.findChild<QPushButton *>(QStringLiteral("importTemplatesButton"));
    auto *exportButton = dialog.findChild<QPushButton *>(QStringLiteral("exportTemplatesButton"));

    assert(combo != nullptr);
    assert(nameEdit != nullptr);
    assert(promptEdit != nullptr);
    assert(importButton != nullptr);
    assert(exportButton != nullptr);
    assert(exportButton->isEnabled());
    assert(combo->count() == 2);
    assert(dialog.prompt() == QStringLiteral("Initial prompt"));

    combo->setCurrentIndex(1);
    assert(nameEdit->text() == QStringLiteral("Test Template"));
    assert(dialog.prompt() == QStringLiteral("You are a test assistant."));
    assert(dialog.templates().size() == 1);

    combo->setCurrentIndex(0);
    assert(nameEdit->text().isEmpty());
    assert(dialog.prompt().isEmpty());

    RolePromptDialog matchedDialog(QStringLiteral("You are a test assistant."), templates, AppLanguage::English);
    auto *matchedCombo = matchedDialog.findChild<QComboBox *>(QStringLiteral("roleTemplateCombo"));
    auto *matchedNameEdit = matchedDialog.findChild<QLineEdit *>(QStringLiteral("roleTemplateNameEdit"));
    assert(matchedCombo != nullptr);
    assert(matchedNameEdit != nullptr);
    assert(matchedCombo->currentIndex() == 1);
    assert(matchedNameEdit->text() == QStringLiteral("Test Template"));
    assert(matchedDialog.prompt() == QStringLiteral("You are a test assistant."));

    return 0;
}
