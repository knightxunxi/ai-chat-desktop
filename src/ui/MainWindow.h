#pragma once

#include "app/ApplicationController.h"
#include "core/ChatSession.h"
#include "core/SessionListFilter.h"

#include <QMainWindow>

class ChatView;
class QCloseEvent;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QLineEdit;
class QTextEdit;

// 学习注释：主窗口 UI 组合层，负责把用户操作转成 ApplicationController 调用，并响应控制层信号刷新界面。
// 使用模块：main.cpp 创建并显示它；业务逻辑应优先放在 ApplicationController，而不是直接堆在这里。
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    // 功能：窗口关闭前取消生成并退出应用；使用模块：用户点关闭、smoke test 自动关闭。
    void closeEvent(QCloseEvent *event) override;

private:
    // 功能：创建侧边栏、聊天区、输入区和顶部按钮；使用模块：构造函数。
    void setupUi();
    // 功能：连接控制层信号到 UI 刷新函数；使用模块：构造函数。
    void connectController();
    // 功能：刷新会话列表；使用模块：sessionListChanged 信号。
    void populateSessionList();
    // 功能：同步当前会话在列表中的选中态；使用模块：会话切换和重命名。
    void updateCurrentSessionListItem();
    // 功能：按会话 ID 查找列表项；使用模块：更新当前项和避免重复插入。
    QListWidgetItem *findSessionItem(const QString &sessionId) const;
    // 功能：生成侧边栏显示标题；使用模块：populateSessionList/updateCurrentSessionListItem。
    QString sessionListTitle(const ChatSession &session) const;
    // 功能：根据当前会话重建聊天区；使用模块：currentSessionChanged 信号。
    void populateChatView();
    // 功能：把配置应用到模型标签；使用模块：configChanged 信号。
    void applyConfig();
    // 功能：刷新所有界面文案；使用模块：语言切换和模板变化。
    void applyLanguage();
    // 功能：同步会话筛选控件；使用模块：语言切换和筛选状态变化。
    void updateSessionFilterButtons();
    // 功能：刷新收藏/归档按钮文案和状态；使用模块：当前会话变化和语言切换。
    void updateSessionOrganizationControls();
    // 功能：根据当前语言选择中文或英文；使用模块：所有 UI 文案。
    QString text(const QString &english, const QString &chinese) const;
    // 功能：计算顶部角色显示名；使用模块：applyLanguage。
    QString currentRoleDisplayName() const;
    // 功能：根据生成状态切换发送/停止按钮样式；使用模块：setGenerating。
    void updateSendButtonAppearance();
    // 功能：根据输入内容和生成状态控制发送按钮启用；使用模块：输入框 textChanged。
    void updateSendButtonState();
    // 功能：打开设置窗口；使用模块：设置按钮。
    void openSettingsDialog();
    // 功能：打开日志窗口；使用模块：日志按钮。
    void openLogViewerDialog();
    // 功能：打开本地工具窗口；使用模块：工具按钮。
    void openToolsDialog();
    // 功能：打开文件工具窗口；使用模块：文件工具按钮。
    void openFileToolsDialog();
    // 功能：把工具输出插入聊天输入框；使用模块：ToolsDialog::outputInsertionRequested 信号。
    void insertToolOutputIntoInput(const QString &output);
    // 功能：打开角色提示词窗口；使用模块：角色提示词按钮。
    void editSystemPrompt();
    // 功能：开始新会话；使用模块：新建会话按钮。
    void startNewChat();
    // 功能：重命名当前会话；使用模块：重命名按钮。
    void renameCurrentChat();
    // 功能：导出当前会话；使用模块：导出按钮。
    void exportCurrentChat();
    // 功能：收藏或取消收藏当前会话；使用模块：收藏按钮。
    void toggleCurrentChatFavorite();
    // 功能：归档或取消归档当前会话；使用模块：归档按钮。
    void toggleCurrentChatArchived();
    // 功能：删除当前会话；使用模块：删除按钮。
    void deleteCurrentChat();
    // 功能：切换会话列表筛选；使用模块：筛选按钮。
    void changeSessionFilter(SessionListFilter filter);
    // 功能：切换到点击的会话；使用模块：侧边栏 itemClicked 信号。
    void switchToSession(QListWidgetItem *item);
    // 功能：发送输入框内容或停止生成；使用模块：发送按钮和快捷键。
    void sendCurrentMessage();
    // 功能：把用户消息增量加入聊天区；使用模块：userMessageAdded 信号。
    void addUserMessage(const QString &content);
    // 功能：添加助手回复占位；使用模块：assistantMessageStarted 信号。
    void addAssistantPlaceholder();
    // 功能：切换生成中 UI 状态；使用模块：generatingChanged 信号。
    void setGenerating(bool generating);
    // 功能：显示或隐藏重试按钮；使用模块：retryAvailableChanged 信号。
    void setRetryAvailable(bool available);
    // 功能：提示用户补全 API 配置；使用模块：configurationMissing 信号。
    void showConfigurationMissingWarning();
    // 功能：显示状态栏消息；使用模块：statusMessage 信号。
    void showStatusMessage(const QString &english, const QString &chinese, int timeoutMs);
    // 功能：启动期延迟弹警告；使用模块：startupWarning 信号。
    void showStartupWarning(const QString &english, const QString &chinese);

    ApplicationController m_controller;      // 功能：主业务控制器；使用模块：主窗口所有用户操作。
    QListWidget *m_sessionList = nullptr;    // 功能：会话列表；使用模块：侧边栏会话展示和切换。
    QLineEdit *m_sessionSearchEdit = nullptr; // 功能：会话搜索输入；使用模块：搜索历史会话。
    QPushButton *m_activeFilterButton = nullptr; // 功能：显示未归档会话；使用模块：侧边栏筛选按钮组。
    QPushButton *m_favoriteFilterButton = nullptr; // 功能：显示收藏会话；使用模块：侧边栏筛选按钮组。
    QPushButton *m_archivedFilterButton = nullptr; // 功能：显示归档会话；使用模块：侧边栏筛选按钮组。
    QPushButton *m_newChatButton = nullptr;  // 功能：新建会话按钮；使用模块：侧边栏操作区。
    QPushButton *m_renameChatButton = nullptr; // 功能：重命名按钮；使用模块：侧边栏操作区。
    QPushButton *m_exportChatButton = nullptr; // 功能：导出按钮；使用模块：侧边栏操作区。
    QPushButton *m_favoriteChatButton = nullptr; // 功能：收藏/取消收藏当前会话；使用模块：侧边栏操作区。
    QPushButton *m_archiveChatButton = nullptr; // 功能：归档/取消归档当前会话；使用模块：侧边栏操作区。
    QPushButton *m_deleteChatButton = nullptr; // 功能：删除按钮；使用模块：侧边栏操作区。
    QPushButton *m_systemPromptButton = nullptr; // 功能：角色提示词入口；使用模块：顶部工具区。
    QPushButton *m_toolsButton = nullptr;     // 功能：本地工具入口；使用模块：顶部工具区。
    QPushButton *m_fileToolsButton = nullptr; // 功能：文件工具入口；使用模块：顶部工具区。
    QPushButton *m_logButton = nullptr;      // 功能：日志窗口入口；使用模块：顶部工具区。
    ChatView *m_chatView = nullptr;          // 功能：聊天消息列表视图；使用模块：主内容区。
    QTextEdit *m_messageInput = nullptr;     // 功能：用户输入框；使用模块：底部输入区。
    QPushButton *m_retryButton = nullptr;    // 功能：失败重试按钮；使用模块：底部输入区。
    QPushButton *m_sendButton = nullptr;     // 功能：发送/停止按钮；使用模块：底部输入区。
    QPushButton *m_settingsButton = nullptr; // 功能：设置入口；使用模块：顶部工具区。
    QLabel *m_modelLabel = nullptr;          // 功能：当前模型展示；使用模块：顶部标题区。
    QLabel *m_personaLabel = nullptr;        // 功能：当前角色展示；使用模块：顶部标题区。
};
