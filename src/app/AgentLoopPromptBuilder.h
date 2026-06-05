#pragma once

#include "core/AppLanguage.h"
#include "skills/SkillDefinition.h"
#include "tools/AgentToolCatalog.h"

#include <QString>
#include <QStringList>
#include <QVector>

namespace AgentLoopPromptBuilder {

// V18.6: 意图类别 — 用于场景匹配工具排序
enum class AgentIntent {
    Unknown,
    CodeEdit,      // 改代码
    CodeSearch,    // 搜代码/理解项目
    BuildTest,     // 构建/测试
    FileManage,    // 文件操作
    DesktopOp,     // 桌面操作
    WebRequest,    // 网络请求
    ShellCmd       // 命令行
};

// 功能：生成 Agentic Loop 单步动作规划提示词；使用模块：V8.2 后续真实 AI 单步规划。
QString buildNextActionPrompt(
    const QString &userGoal,
    const QStringList &observations,
    const QVector<AgentToolDescriptor> &toolCatalog,
    AppLanguage language,
    int completedSteps,
    int maxSteps,
    const QVector<SkillDefinition> &matchedSkills = {});

// V18.6: 从用户目标中检测意图类别
AgentIntent classifyGoal(const QString &userGoal);

// V18.6: 按意图优先排序工具（匹配的排前面加 ⭐，不匹配的排后面）
QVector<AgentToolDescriptor> reorderToolsByIntent(
    const QVector<AgentToolDescriptor> &catalog,
    AgentIntent intent,
    AppLanguage language);

// V18.6: 构建工具使用指导（从记忆生成）
QString buildToolGuidance(const QString &userGoal,
                          const QVector<AgentToolDescriptor> &reorderedTools);

// V18.6: 记录成功的工具序列到记忆
void recordToolSequence(const QString &projectDir,
                        const QString &goalSummary,
                        const QStringList &toolIds);

} // namespace AgentLoopPromptBuilder
