#pragma once

#include "core/AppConfig.h"

#include <QCheckBox>
#include <QDialog>

class QComboBox;
class QDialogButtonBox;
class QFormLayout;
class QLabel;
class QLineEdit;

// 学习注释：应用设置窗口，负责编辑服务商、Base URL、模型、API Key、模型参数、Agent 工作目录、项目目录和界面语言。
// 使用模块：MainWindow 打开它，ApplicationController 保存它返回的 AppConfig。
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(const AppConfig &config, QWidget *parent = nullptr);

    // 功能：返回窗口中编辑后的配置；使用模块：MainWindow::openSettingsDialog。
    AppConfig config() const;

    // V16.3: Agent 调试模式 getter/setter（不在 AppConfig 中持久化）
    bool debugMode() const { return m_debugMode; }
    void setDebugMode(bool enabled) { m_debugMode = enabled; }

private:
    // 功能：创建表单控件和布局；使用模块：构造函数。
    void setupUi();
    // 功能：根据语言刷新标签和按钮文案；使用模块：语言切换。
    void applyTexts();
    // 功能：应用服务商预设到 Base URL 和模型名；使用模块：服务商下拉框切换。
    void applySelectedProviderPreset(int index);
    // 功能：控制自定义服务商输入框显隐；使用模块：服务商下拉框切换。
    void updateCustomProviderVisibility();
    // 功能：确认前做字段整理和校验；使用模块：点击 OK/确认按钮。
    void accept() override;

    AppConfig m_initialConfig;              // 功能：进入窗口时的配置快照；使用模块：初始化表单和语言。
    QFormLayout *m_formLayout = nullptr;    // 功能：设置表单布局；使用模块：setupUi。
    QLabel *m_titleLabel = nullptr;         // 功能：窗口标题文案；使用模块：applyTexts。
    QLabel *m_providerLabel = nullptr;      // 功能：服务商字段标签；使用模块：applyTexts。
    QLabel *m_customProviderLabel = nullptr; // 功能：自定义服务商标签；使用模块：自定义服务商模式。
    QLabel *m_baseUrlLabel = nullptr;       // 功能：Base URL 标签；使用模块：applyTexts。
    QLabel *m_modelNameLabel = nullptr;     // 功能：模型名标签；使用模块：applyTexts。
    QLabel *m_apiKeyLabel = nullptr;        // 功能：API Key 标签；使用模块：applyTexts。
    QLabel *m_temperatureLabel = nullptr;   // 功能：temperature 标签；使用模块：可选模型参数。
    QLabel *m_maxTokensLabel = nullptr;     // 功能：max_tokens 标签；使用模块：可选模型参数。
    QLabel *m_agentWorkspaceLabel = nullptr; // 功能：Agent 工作目录标签；使用模块：后续文件生成默认路径。
    QLabel *m_agentProjectLabel = nullptr;  // 功能：Agent 项目目录标签；使用模块：V9 命令执行。
    QLabel *m_languageLabel = nullptr;      // 功能：语言标签；使用模块：applyTexts。
    QComboBox *m_providerCombo = nullptr;   // 功能：服务商选择；使用模块：预设填充配置。
    QLineEdit *m_customProviderEdit = nullptr; // 功能：自定义服务商名称；使用模块：选择 Custom 时启用。
    QLineEdit *m_baseUrlEdit = nullptr;     // 功能：Base URL 输入；使用模块：生成 AppConfig。
    QLineEdit *m_modelNameEdit = nullptr;   // 功能：模型名输入；使用模块：生成 AppConfig。
    QLineEdit *m_apiKeyEdit = nullptr;      // 功能：API Key 输入；使用模块：生成 AppConfig 后安全保存。
    QLineEdit *m_temperatureEdit = nullptr; // 功能：temperature 输入；使用模块：可选模型参数。
    QLineEdit *m_maxTokensEdit = nullptr;   // 功能：max_tokens 输入；使用模块：可选模型参数。
    QLineEdit *m_agentWorkspaceEdit = nullptr; // 功能：Agent 工作目录输入；使用模块：生成 AppConfig。
    QLineEdit *m_agentProjectEdit = nullptr; // 功能：Agent 项目目录输入；使用模块：生成 AppConfig。
    QComboBox *m_languageCombo = nullptr;   // 功能：语言选择；使用模块：界面语言切换。
    QDialogButtonBox *m_buttonBox = nullptr; // 功能：确认/取消按钮；使用模块：标准对话框操作。

    // V16.3: Agent 调试模式（移到设置界面）
    bool m_debugMode = false;
    QCheckBox *m_debugModeCheckbox = nullptr;
};
