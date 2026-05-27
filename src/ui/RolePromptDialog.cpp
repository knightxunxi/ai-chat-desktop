#include "ui/RolePromptDialog.h"

#include "storage/PromptTemplateStorage.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {

constexpr auto TemplateIdRole = Qt::UserRole;

} // namespace

RolePromptDialog::RolePromptDialog(const QString &currentPrompt,
                                   const QVector<PromptTemplate> &templates,
                                   AppLanguage language,
                                   QWidget *parent)
    : QDialog(parent)
    , m_language(language)
    , m_templates(templates)
    , m_initialPrompt(currentPrompt)
{
    setupUi();
    const QString selectedId = matchingTemplateIdForPrompt(m_initialPrompt);
    refreshTemplateCombo(selectedId);
    if (selectedId.isEmpty()) {
        m_promptEdit->setPlainText(m_initialPrompt);
    } else {
        applySelectedTemplate(m_templateCombo->currentIndex());
    }
}

QString RolePromptDialog::prompt() const
{
    return m_promptEdit->toPlainText().trimmed();
}

QVector<PromptTemplate> RolePromptDialog::templates() const
{
    return m_templates;
}

QString RolePromptDialog::text(const QString &english, const QString &chinese) const
{
    return m_language == AppLanguage::English ? english : chinese;
}

void RolePromptDialog::setupUi()
{
    setWindowTitle(text(QStringLiteral("Role Prompt"), QStringLiteral("角色提示词")));
    resize(620, 460);
    setMinimumSize(520, 380);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(14);

    auto *title = new QLabel(text(QStringLiteral("Role Prompt Templates"), QStringLiteral("角色提示词模板")), this);
    title->setObjectName(QStringLiteral("settingsTitle"));

    auto *formLayout = new QFormLayout();
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(10);

    m_templateCombo = new QComboBox(this);
    m_templateCombo->setObjectName(QStringLiteral("roleTemplateCombo"));

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setObjectName(QStringLiteral("roleTemplateNameEdit"));
    m_nameEdit->setPlaceholderText(text(QStringLiteral("Template name"), QStringLiteral("模板名称")));

    m_promptEdit = new QTextEdit(this);
    m_promptEdit->setObjectName(QStringLiteral("rolePromptEdit"));
    m_promptEdit->setMinimumHeight(220);
    m_promptEdit->setPlaceholderText(text(QStringLiteral("Set the system prompt for this chat..."),
                                          QStringLiteral("设置当前会话的系统提示词...")));

    formLayout->addRow(text(QStringLiteral("Template"), QStringLiteral("模板")), m_templateCombo);
    formLayout->addRow(text(QStringLiteral("Name"), QStringLiteral("名称")), m_nameEdit);
    formLayout->addRow(text(QStringLiteral("Prompt"), QStringLiteral("提示词")), m_promptEdit);

    m_saveTemplateButton = new QPushButton(text(QStringLiteral("Save Template"), QStringLiteral("保存模板")), this);
    m_deleteTemplateButton = new QPushButton(text(QStringLiteral("Delete Template"), QStringLiteral("删除模板")), this);
    m_importTemplatesButton = new QPushButton(text(QStringLiteral("Import"), QStringLiteral("导入")), this);
    m_importTemplatesButton->setObjectName(QStringLiteral("importTemplatesButton"));
    m_exportTemplatesButton = new QPushButton(text(QStringLiteral("Export"), QStringLiteral("导出")), this);
    m_exportTemplatesButton->setObjectName(QStringLiteral("exportTemplatesButton"));
    m_clearPromptButton = new QPushButton(text(QStringLiteral("Clear Prompt"), QStringLiteral("清空提示词")), this);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(text(QStringLiteral("Apply"), QStringLiteral("应用")));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(text(QStringLiteral("Cancel"), QStringLiteral("取消")));

    auto *bottomActions = new QHBoxLayout();
    bottomActions->setContentsMargins(0, 0, 0, 0);
    bottomActions->setSpacing(10);
    bottomActions->addWidget(m_saveTemplateButton);
    bottomActions->addWidget(m_deleteTemplateButton);
    bottomActions->addWidget(m_importTemplatesButton);
    bottomActions->addWidget(m_exportTemplatesButton);
    bottomActions->addWidget(m_clearPromptButton);
    bottomActions->addStretch(1);
    bottomActions->addWidget(buttonBox);

    layout->addWidget(title);
    layout->addLayout(formLayout);
    layout->addLayout(bottomActions);

    connect(m_templateCombo, &QComboBox::currentIndexChanged, this, &RolePromptDialog::applySelectedTemplate);
    connect(m_saveTemplateButton, &QPushButton::clicked, this, &RolePromptDialog::saveCurrentTemplate);
    connect(m_deleteTemplateButton, &QPushButton::clicked, this, &RolePromptDialog::deleteSelectedTemplate);
    connect(m_importTemplatesButton, &QPushButton::clicked, this, &RolePromptDialog::importTemplates);
    connect(m_exportTemplatesButton, &QPushButton::clicked, this, &RolePromptDialog::exportTemplates);
    connect(m_clearPromptButton, &QPushButton::clicked, this, &RolePromptDialog::clearPrompt);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &RolePromptDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &RolePromptDialog::reject);
}

void RolePromptDialog::refreshTemplateCombo(const QString &selectedId)
{
    const QSignalBlocker blocker(m_templateCombo);
    m_templateCombo->clear();
    m_templateCombo->addItem(text(QStringLiteral("Custom prompt"), QStringLiteral("自定义提示词")), QString());

    int selectedIndex = 0;
    for (const PromptTemplate &promptTemplate : m_templates) {
        if (!promptTemplate.isValid()) {
            continue;
        }

        m_templateCombo->addItem(promptTemplate.name, promptTemplate.id);
        if (!selectedId.isEmpty() && promptTemplate.id == selectedId) {
            selectedIndex = m_templateCombo->count() - 1;
        }
    }

    m_templateCombo->setCurrentIndex(selectedIndex);
    updateTemplateActions();
}

int RolePromptDialog::findTemplateIndex(const QString &id) const
{
    if (id.trimmed().isEmpty()) {
        return -1;
    }

    for (int index = 0; index < m_templates.size(); ++index) {
        if (m_templates[index].id == id) {
            return index;
        }
    }

    return -1;
}

QString RolePromptDialog::matchingTemplateIdForPrompt(const QString &prompt) const
{
    const QString normalizedPrompt = prompt.trimmed();
    if (normalizedPrompt.isEmpty()) {
        return QString();
    }

    for (const PromptTemplate &promptTemplate : m_templates) {
        if (promptTemplate.content.trimmed() == normalizedPrompt) {
            return promptTemplate.id;
        }
    }

    return QString();
}

QString RolePromptDialog::selectedTemplateId() const
{
    return m_templateCombo->currentData(TemplateIdRole).toString();
}

void RolePromptDialog::applySelectedTemplate(int index)
{
    if (index < 0) {
        updateTemplateActions();
        return;
    }

    const QString id = selectedTemplateId();
    const int templateIndex = findTemplateIndex(id);
    if (templateIndex < 0) {
        m_nameEdit->clear();
        m_promptEdit->clear();
        updateTemplateActions();
        return;
    }

    m_nameEdit->setText(m_templates[templateIndex].name);
    m_promptEdit->setPlainText(m_templates[templateIndex].content);
    updateTemplateActions();
}

void RolePromptDialog::saveCurrentTemplate()
{
    const QString name = m_nameEdit->text().trimmed();
    const QString content = m_promptEdit->toPlainText().trimmed();
    if (name.isEmpty() || content.isEmpty()) {
        QMessageBox::warning(this,
                             text(QStringLiteral("Template incomplete"), QStringLiteral("模板不完整")),
                             text(QStringLiteral("Please enter both a template name and prompt content."),
                                  QStringLiteral("请输入模板名称和提示词内容。")));
        return;
    }

    const QString id = selectedTemplateId();
    const int templateIndex = findTemplateIndex(id);
    if (templateIndex >= 0) {
        m_templates[templateIndex].name = name;
        m_templates[templateIndex].content = content;
        refreshTemplateCombo(id);
        return;
    }

    const PromptTemplate promptTemplate = PromptTemplate::create(name, content);
    m_templates.append(promptTemplate);
    refreshTemplateCombo(promptTemplate.id);
}

void RolePromptDialog::deleteSelectedTemplate()
{
    const int templateIndex = findTemplateIndex(selectedTemplateId());
    if (templateIndex < 0) {
        return;
    }

    m_templates.removeAt(templateIndex);
    refreshTemplateCombo();
    m_nameEdit->clear();
    m_promptEdit->clear();
}

void RolePromptDialog::importTemplates()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        text(QStringLiteral("Import prompt templates"), QStringLiteral("导入角色提示词模板")),
        QString(),
        QStringLiteral("JSON (*.json)"));
    if (filePath.trimmed().isEmpty()) {
        return;
    }

    QString error;
    const QVector<PromptTemplate> importedTemplates = PromptTemplateStorage::importTemplates(filePath, &error);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, text(QStringLiteral("Import failed"), QStringLiteral("导入失败")), error);
        return;
    }

    if (importedTemplates.isEmpty()) {
        QMessageBox::information(this,
                                 text(QStringLiteral("Import prompt templates"), QStringLiteral("导入角色提示词模板")),
                                 text(QStringLiteral("No valid prompt templates were found."),
                                      QStringLiteral("未找到有效的角色提示词模板。")));
        return;
    }

    const int previousCount = m_templates.size();
    m_templates = PromptTemplateStorage::mergeTemplates(m_templates, importedTemplates);
    refreshTemplateCombo();

    QMessageBox::information(this,
                             text(QStringLiteral("Import prompt templates"), QStringLiteral("导入角色提示词模板")),
                             text(QStringLiteral("Imported %1 prompt template(s)."),
                                  QStringLiteral("已导入 %1 个角色提示词模板。"))
                                 .arg(m_templates.size() - previousCount));
}

void RolePromptDialog::exportTemplates()
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        text(QStringLiteral("Export prompt templates"), QStringLiteral("导出角色提示词模板")),
        QStringLiteral("prompt-templates.json"),
        QStringLiteral("JSON (*.json)"));
    if (filePath.trimmed().isEmpty()) {
        return;
    }

    if (QFileInfo(filePath).suffix().isEmpty()) {
        filePath += QStringLiteral(".json");
    }

    QString error;
    if (!PromptTemplateStorage::exportTemplates(filePath, m_templates, &error)) {
        QMessageBox::warning(this, text(QStringLiteral("Export failed"), QStringLiteral("导出失败")), error);
        return;
    }

    QMessageBox::information(this,
                             text(QStringLiteral("Export prompt templates"), QStringLiteral("导出角色提示词模板")),
                             text(QStringLiteral("Prompt templates were exported."),
                                  QStringLiteral("角色提示词模板已导出。")));
}

void RolePromptDialog::clearPrompt()
{
    m_templateCombo->setCurrentIndex(0);
    m_nameEdit->clear();
    m_promptEdit->clear();
}

void RolePromptDialog::updateTemplateActions()
{
    m_deleteTemplateButton->setEnabled(findTemplateIndex(selectedTemplateId()) >= 0);
    m_exportTemplatesButton->setEnabled(!m_templates.isEmpty());
}
