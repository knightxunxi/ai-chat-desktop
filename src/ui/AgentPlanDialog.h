#pragma once

#include "app/AgentPlan.h"
#include "core/AppConfig.h"
#include "core/AppLanguage.h"
#include "tools/AgentToolCatalog.h"

#include <QDialog>
#include <QVector>

class AIClient;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QPushButton;

// 学习注释：Agent 计划预览窗口，展示 AI 计划并要求用户逐步确认或跳过。
// 使用模块：MainWindow 接收 ApplicationController 生成的计划后打开。
class AgentPlanDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AgentPlanDialog(
        const AgentPlan &plan,
        const QVector<AgentToolDescriptor> &toolCatalog,
        AppLanguage language,
        QWidget *parent = nullptr,
        const QString &workspaceDirectory = QString(),
        const QString &projectDirectory = QString(),
        AIClient *aiClient = nullptr,
        const AppConfig &appConfig = AppConfig());

signals:
    // 功能：请求把当前步骤输出插入聊天输入框；使用模块：MainWindow。
    void outputInsertionRequested(const QString &output);
    // 功能：用户确认后请求基于当前输出继续生成计划；使用模块：MainWindow。
    void continuePlanningRequested(const QString &goal, int continuationDepth);

private:
    // 功能：创建计划预览 UI；使用模块：构造函数。
    void setupUi();
    // 功能：刷新中英文文案；使用模块：构造函数。
    void applyLanguage();
    // 功能：填充步骤列表；使用模块：构造函数和步骤状态变化后。
    void populateStepList();
    // 功能：刷新当前步骤详情；使用模块：步骤选择变化。
    void updateSelectedStepDetails();
    // 功能：执行当前选中步骤；使用模块：执行按钮。
    void executeSelectedStep();
    // 功能：连续执行可直接执行的待处理步骤；使用模块：V8 Agent 连续执行按钮。
    void executeRemainingSteps();
    // 功能：启动无限 Agentic 循环（AI 自主决定每步）；使用模块：V12.2 无限循环模式按钮。
    void executeInfiniteLoop();
    // 功能：请求停止连续执行；使用模块：停止按钮。
    void requestStopExecution();
    // 功能：跳过当前选中步骤；使用模块：跳过按钮。
    void skipSelectedStep();
    // 功能：复制当前输出；使用模块：复制按钮。
    void copyOutput();
    // 功能：插入当前输出到聊天输入框；使用模块：插入按钮。
    void insertOutput();
    // 功能：基于当前输出继续规划；使用模块：继续规划按钮。
    void continuePlanning();
    // 功能：刷新按钮状态；使用模块：步骤选择和输出变化后。
    void updateActionButtons();
    // 功能：执行指定步骤并刷新 UI；使用模块：单步执行和连续执行。
    bool executeStepAt(int index);
    // 功能：查找下一个可直接执行的待处理步骤；使用模块：连续执行。
    int nextExecutableStepIndex() const;
    // 功能：返回当前选中步骤索引；使用模块：执行/跳过/详情刷新。
    int selectedStepIndex() const;
    // 功能：生成步骤列表显示文本；使用模块：populateStepList。
    QString stepListText(const AgentPlanStep &step) const;
    // 功能：生成当前步骤详情文本；使用模块：updateSelectedStepDetails。
    QString stepDetailsText(const AgentPlanStep &step) const;
    // 功能：查找工具描述；使用模块：详情显示。
    const AgentToolDescriptor *toolDescriptorForStep(const AgentPlanStep &step) const;
    // 功能：根据语言选择文案；使用模块：所有 UI 文案。
    QString text(const QString &english, const QString &chinese) const;

    AgentPlan m_plan;                              // 功能：当前计划和步骤状态；使用模块：列表与执行。
    QVector<AgentToolDescriptor> m_toolCatalog;    // 功能：工具目录；使用模块：显示工具名称和风险。
    AppLanguage m_language = AppLanguage::Chinese; // 功能：窗口语言；使用模块：文案选择。
    QString m_workspaceDirectory;                  // 功能：Agent 工作目录；使用模块：workspace.* 步骤执行。
    QString m_projectDirectory;                    // 功能：项目命令工作目录；使用模块：command.* 步骤执行。
    AIClient *m_aiClient = nullptr;                // 功能：AI 客户端；使用模块：executeInfiniteLoop AI 调用。
    AppConfig m_appConfig;                         // 功能：AI 配置（模型参数等）；使用模块：executeInfiniteLoop 配置。
    QLabel *m_summaryLabel = nullptr;              // 功能：计划摘要；使用模块：顶部展示。
    QLabel *m_statusLabel = nullptr;               // 功能：执行状态提示；使用模块：执行/跳过。
    QListWidget *m_stepList = nullptr;             // 功能：计划步骤列表；使用模块：用户选择步骤。
    QPlainTextEdit *m_detailEdit = nullptr;        // 功能：步骤详情；使用模块：显示参数和风险。
    QPlainTextEdit *m_outputEdit = nullptr;        // 功能：步骤输出；使用模块：执行后展示。
    QPushButton *m_executeButton = nullptr;        // 功能：执行选中步骤；使用模块：用户确认。
    QPushButton *m_runAllButton = nullptr;         // 功能：连续执行待处理步骤；使用模块：V8 连续执行 MVP。
    QPushButton *m_stopButton = nullptr;           // 功能：请求停止连续执行；使用模块：V8 连续执行 MVP。
    QPushButton *m_loopButton = nullptr;           // 功能：启动无限 Agentic 循环；使用模块：V12.2 无限循环模式。
    QPushButton *m_skipButton = nullptr;           // 功能：跳过选中步骤；使用模块：用户跳过。
    QPushButton *m_copyButton = nullptr;           // 功能：复制输出；使用模块：输出处理。
    QPushButton *m_insertButton = nullptr;         // 功能：插入输出；使用模块：输出处理。
    QPushButton *m_continueButton = nullptr;       // 功能：基于输出继续规划；使用模块：用户确认回传输出。
    QPushButton *m_closeButton = nullptr;          // 功能：关闭窗口；使用模块：结束计划预览。
    bool m_runningBatch = false;                   // 功能：是否正在连续执行；使用模块：按钮状态和停止逻辑。
    bool m_stopRequested = false;                  // 功能：停止请求标记；使用模块：连续执行循环。
};
