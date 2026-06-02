#pragma once

#include "core/AppLanguage.h"

#include <QDialog>

class QLabel;
class QPlainTextEdit;
class QPushButton;

// 学习注释：受控文件工具窗口，负责用户主动选择文件/目录并执行低风险本地交互。
// 使用模块：MainWindow 打开该窗口；FileInteractionService 执行实际文件操作。
class FileToolsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FileToolsDialog(AppLanguage language, QWidget *parent = nullptr);

signals:
    // 功能：请求把文件工具输出插入聊天输入框；使用模块：MainWindow 接收后写入 m_messageInput。
    void outputInsertionRequested(const QString &output);

private:
    // 功能：创建文件工具窗口控件；使用模块：构造函数。
    void setupUi();
    // 功能：刷新窗口文案；使用模块：构造函数。
    void applyLanguage();
    // 功能：读取用户选择的文本文件；使用模块：读取文件按钮。
    void readSelectedFile();
    // 功能：列出用户选择的目录；使用模块：列出目录按钮。
    void listSelectedDirectory();
    // 功能：把当前输出保存到用户选择的文件；使用模块：保存输出按钮。
    void saveOutputToFile();
    // 功能：打开用户选择的文件；使用模块：打开文件按钮。
    void openSelectedFile();
    // 功能：打开用户选择的目录；使用模块：打开目录按钮。
    void openSelectedDirectory();
    // 功能：复制当前输出；使用模块：复制按钮。
    void copyOutput();
    // 功能：把当前输出插入聊天输入框；使用模块：插入按钮。
    void insertOutput();
    // 功能：根据输出是否可用刷新按钮状态；使用模块：文件操作完成后。
    void updateOutputActions(bool enabled);
    // 功能：显示工具输出和状态；使用模块：读取/列目录成功后。
    void setOutput(const QString &output, const QString &status);
    // 功能：验证并打开指定路径；使用模块：打开文件/目录按钮。
    void confirmAndOpenPath(const QString &path);
    // 功能：根据当前语言选择中文或英文；使用模块：所有 UI 文案。
    QString text(const QString &english, const QString &chinese) const;

    AppLanguage m_language = AppLanguage::Chinese; // 功能：窗口语言；使用模块：按钮和提示文案。
    QLabel *m_titleLabel = nullptr;                // 功能：窗口标题；使用模块：applyLanguage。
    QLabel *m_descriptionLabel = nullptr;          // 功能：安全边界说明；使用模块：applyLanguage。
    QLabel *m_statusLabel = nullptr;               // 功能：运行状态和错误提示；使用模块：各操作函数。
    QPlainTextEdit *m_outputEdit = nullptr;        // 功能：文件工具输出区；使用模块：复制、保存、插入。
    QPushButton *m_readFileButton = nullptr;       // 功能：读取文件；使用模块：readSelectedFile。
    QPushButton *m_listDirectoryButton = nullptr;  // 功能：列目录；使用模块：listSelectedDirectory。
    QPushButton *m_saveOutputButton = nullptr;     // 功能：保存输出；使用模块：saveOutputToFile。
    QPushButton *m_openFileButton = nullptr;       // 功能：打开文件；使用模块：openSelectedFile。
    QPushButton *m_openDirectoryButton = nullptr;  // 功能：打开目录；使用模块：openSelectedDirectory。
    QPushButton *m_copyButton = nullptr;           // 功能：复制输出；使用模块：copyOutput。
    QPushButton *m_insertButton = nullptr;         // 功能：插入输出；使用模块：insertOutput。
    QPushButton *m_closeButton = nullptr;          // 功能：关闭窗口；使用模块：用户关闭文件工具窗口。
};
