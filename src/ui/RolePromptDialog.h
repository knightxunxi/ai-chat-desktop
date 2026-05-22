#pragma once

#include "core/AppLanguage.h"
#include "core/PromptTemplate.h"

#include <QDialog>
#include <QVector>

class QComboBox;
class QLineEdit;
class QPushButton;
class QTextEdit;

class RolePromptDialog : public QDialog
{
    Q_OBJECT

public:
    RolePromptDialog(const QString &currentPrompt,
                     const QVector<PromptTemplate> &templates,
                     AppLanguage language,
                     QWidget *parent = nullptr);

    QString prompt() const;
    QVector<PromptTemplate> templates() const;

private:
    QString text(const QString &english, const QString &chinese) const;
    void setupUi();
    void refreshTemplateCombo(const QString &selectedId = QString());
    int findTemplateIndex(const QString &id) const;
    QString matchingTemplateIdForPrompt(const QString &prompt) const;
    QString selectedTemplateId() const;
    void applySelectedTemplate(int index);
    void saveCurrentTemplate();
    void deleteSelectedTemplate();
    void clearPrompt();
    void updateTemplateActions();

    AppLanguage m_language = AppLanguage::Chinese;
    QVector<PromptTemplate> m_templates;
    QString m_initialPrompt;
    QComboBox *m_templateCombo = nullptr;
    QLineEdit *m_nameEdit = nullptr;
    QTextEdit *m_promptEdit = nullptr;
    QPushButton *m_saveTemplateButton = nullptr;
    QPushButton *m_deleteTemplateButton = nullptr;
    QPushButton *m_clearPromptButton = nullptr;
};
