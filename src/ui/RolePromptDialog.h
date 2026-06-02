#pragma once

#include "core/AppLanguage.h"
#include "core/PromptTemplate.h"

#include <QDialog>
#include <QVector>

class QComboBox;
class QLineEdit;
class QPushButton;
class QTextEdit;

// 学习注释：角色提示词编辑窗口，支持选择模板、自定义提示词、保存/删除模板和 JSON 导入导出。
// 使用模块：MainWindow 点击“角色提示词”时打开，ApplicationController 接收窗口返回的 prompt 和模板列表。
class RolePromptDialog : public QDialog
{
    Q_OBJECT

public:
    RolePromptDialog(const QString &currentPrompt,
                     const QVector<PromptTemplate> &templates,
                     AppLanguage language,
                     QWidget *parent = nullptr);

    // 功能：返回用户最终应用的提示词；使用模块：MainWindow::editSystemPrompt。
    QString prompt() const;
    // 功能：返回用户编辑后的模板列表；使用模块：ApplicationController::savePromptTemplates。
    QVector<PromptTemplate> templates() const;

private:
    // 功能：根据语言选择文案；使用模块：窗口内所有标签和按钮。
    QString text(const QString &english, const QString &chinese) const;
    // 功能：创建窗口控件和布局；使用模块：构造函数。
    void setupUi();
    // 功能：刷新模板下拉框；使用模块：保存、删除、导入模板后。
    void refreshTemplateCombo(const QString &selectedId = QString());
    // 功能：按模板 ID 查找下拉项；使用模块：refreshTemplateCombo。
    int findTemplateIndex(const QString &id) const;
    // 功能：根据当前 prompt 找到匹配模板；使用模块：打开窗口时定位当前角色。
    QString matchingTemplateIdForPrompt(const QString &prompt) const;
    // 功能：返回当前下拉框选中模板 ID；使用模块：保存、删除、应用模板。
    QString selectedTemplateId() const;
    // 功能：把选中模板内容填入编辑区；使用模块：模板下拉框切换。
    void applySelectedTemplate(int index);
    // 功能：保存当前名称和提示词为模板；使用模块：保存按钮。
    void saveCurrentTemplate();
    // 功能：删除当前模板；使用模块：删除按钮。
    void deleteSelectedTemplate();
    // 功能：导入模板 JSON；使用模块：导入按钮。
    void importTemplates();
    // 功能：导出模板 JSON；使用模块：导出按钮。
    void exportTemplates();
    // 功能：清空当前提示词编辑区；使用模块：清空按钮。
    void clearPrompt();
    // 功能：更新保存/删除/导出按钮状态；使用模块：模板或文本变化后。
    void updateTemplateActions();

    AppLanguage m_language = AppLanguage::Chinese; // 功能：窗口语言；使用模块：text。
    QVector<PromptTemplate> m_templates;           // 功能：当前模板列表副本；使用模块：保存/删除/导入/导出。
    QString m_initialPrompt;                       // 功能：打开窗口时的提示词；使用模块：初始化下拉框和编辑区。
    QComboBox *m_templateCombo = nullptr;          // 功能：模板选择下拉框；使用模块：切换模板。
    QLineEdit *m_nameEdit = nullptr;               // 功能：模板名称输入框；使用模块：保存模板。
    QTextEdit *m_promptEdit = nullptr;             // 功能：提示词内容编辑区；使用模块：应用到会话。
    QPushButton *m_saveTemplateButton = nullptr;   // 功能：保存模板按钮；使用模块：模板管理区。
    QPushButton *m_deleteTemplateButton = nullptr; // 功能：删除模板按钮；使用模块：模板管理区。
    QPushButton *m_importTemplatesButton = nullptr; // 功能：导入 JSON 按钮；使用模块：模板迁移。
    QPushButton *m_exportTemplatesButton = nullptr; // 功能：导出 JSON 按钮；使用模块：模板迁移。
    QPushButton *m_clearPromptButton = nullptr;    // 功能：清空提示词按钮；使用模块：编辑区快捷操作。
};
