#pragma once

#include "core/AppLanguage.h"
#include "tools/LocalTool.h"

#include <QDialog>

#include <memory>
#include <vector>

class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;

// 学习注释：本地工具窗口，负责选择工具、运行文本转换并把结果交给主窗口。
// 使用模块：MainWindow 打开该窗口；JsonFormatTool/JsonCompactTool 作为内置工具被它调用。
class ToolsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ToolsDialog(AppLanguage language, QWidget *parent = nullptr);

signals:
    // 功能：请求把工具输出插入聊天输入框；使用模块：MainWindow 接收后写入 m_messageInput。
    void outputInsertionRequested(const QString &output);

private:
    // 功能：创建工具窗口控件；使用模块：构造函数。
    void setupUi();
    // 功能：注册当前内置工具；使用模块：构造函数。
    void registerTools();
    // 功能：刷新窗口文案；使用模块：构造函数。
    void applyLanguage();
    // 功能：根据选择的工具刷新说明；使用模块：工具下拉框切换。
    void updateSelectedToolDescription();
    // 功能：运行当前选择的工具；使用模块：运行按钮。
    void runSelectedTool();
    // 功能：复制工具输出；使用模块：复制按钮。
    void copyOutput();
    // 功能：把工具输出发送给主窗口；使用模块：插入按钮。
    void insertOutput();
    // 功能：根据当前语言选择中文或英文；使用模块：所有 UI 文案。
    QString text(const QString &english, const QString &chinese) const;
    // 功能：读取当前工具；使用模块：运行和说明刷新。
    const LocalTool *selectedTool() const;
    // 功能：根据输出是否可用刷新按钮状态；使用模块：运行工具后。
    void updateOutputActions(bool enabled);

    AppLanguage m_language = AppLanguage::Chinese; // 功能：窗口语言；使用模块：工具名称和按钮文案。
    std::vector<std::unique_ptr<LocalTool>> m_tools; // 功能：内置工具集合；使用模块：下拉框选择和运行。
    QLabel *m_titleLabel = nullptr;                 // 功能：窗口标题；使用模块：applyLanguage。
    QLabel *m_descriptionLabel = nullptr;           // 功能：当前工具说明；使用模块：updateSelectedToolDescription。
    QLabel *m_statusLabel = nullptr;                // 功能：运行状态或错误提示；使用模块：runSelectedTool。
    QComboBox *m_toolComboBox = nullptr;            // 功能：工具选择；使用模块：selectedTool。
    QPlainTextEdit *m_inputEdit = nullptr;          // 功能：工具输入区；使用模块：runSelectedTool。
    QPlainTextEdit *m_outputEdit = nullptr;         // 功能：工具输出区；使用模块：复制和插入。
    QPushButton *m_runButton = nullptr;             // 功能：运行工具；使用模块：runSelectedTool。
    QPushButton *m_copyButton = nullptr;            // 功能：复制输出；使用模块：copyOutput。
    QPushButton *m_insertButton = nullptr;          // 功能：插入输出；使用模块：insertOutput。
    QPushButton *m_closeButton = nullptr;           // 功能：关闭窗口；使用模块：用户关闭工具窗口。
};
