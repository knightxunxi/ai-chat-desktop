# V7 验收记录

本文档记录 AI Chat Desktop V7 阶段当前开发分支的完成情况、验证结果和剩余手工验收项。

执行状态：V7-TASK-001 至 V7-TASK-008 已完成第一版。当前分支已经具备受控 Agent 雏形，但真实 AI 计划生成仍需要使用已配置 API Key 按 [V7 手工验证脚本](25-v7-manual-test-script.md) 进行手工验证。

## 1. 完成范围

V7 当前完成了以下能力：

- Agent 安全设计。
- Agent 工具目录。
- 结构化计划模型。
- AI 计划 JSON 解析和本地校验。
- 常见 Markdown 代码块包裹 JSON 的提取和校验。
- 计划 Prompt 生成器。
- 主窗口“Agent 计划”入口。
- 真实 AI 计划请求入口。
- 计划预览窗口。
- 用户确认后的低风险文本工具单步执行。
- 步骤输出复制。
- 步骤输出插入聊天输入框。
- 基于用户确认后的步骤输出继续规划。
- 继续规划轮次限制。
- 自动化测试补强。

## 2. 任务完成情况

| 任务 | 状态 | 说明 |
| --- | --- | --- |
| V7-TASK-001 Agent 安全设计 | 完成 | 新增 V7 Agent 安全设计文档 |
| V7-TASK-002 工具目录设计 | 完成 | 新增 `AgentToolCatalog` |
| V7-TASK-003 计划模型和解析器 | 完成 | 新增 `AgentPlan` 和 `AgentPlanParser` |
| V7-TASK-004 AI 计划生成 | 完成第一版 | 新增计划 Prompt 和真实 AI 请求入口；需手工验证真实模型输出 |
| V7-TASK-005 计划预览 UI | 完成 | 新增 `AgentPlanDialog` |
| V7-TASK-006 单步工具执行 | 完成 | 低风险文本工具可在用户确认后执行 |
| V7-TASK-007 结果回传和继续规划 | 完成第一版 | 输出可插入聊天输入框；用户确认后可继续规划 |
| V7-TASK-008 V7 验收和项目展示更新 | 完成第一版 | 更新 README、Roadmap、learn 文档和本验收记录 |

## 3. 新增代码能力

新增 Agent 数据和流程模块：

```text
src/app/AgentPlan.h
src/app/AgentPlan.cpp
src/app/AgentPlanParser.h
src/app/AgentPlanParser.cpp
src/app/AgentPlanPromptBuilder.h
src/app/AgentPlanPromptBuilder.cpp
src/app/AgentPlanExecutor.h
src/app/AgentPlanExecutor.cpp
src/tools/AgentToolCatalog.h
src/tools/AgentToolCatalog.cpp
```

新增 UI：

```text
src/ui/AgentPlanDialog.h
src/ui/AgentPlanDialog.cpp
```

主窗口新增：

- “Agent 计划”按钮。
- 计划生成状态提示。
- 计划生成成功后打开计划预览窗口。
- 计划输出插入聊天输入框。
- 继续规划请求转回控制层。

## 4. 安全边界

V7 当前遵守以下边界：

- AI 只生成计划，不直接执行工具。
- 计划必须通过本地 JSON 解析器校验。
- 工具 ID 必须存在于工具目录。
- 步骤数量受上限限制。
- 工具风险等级以本地工具目录为准。
- 低风险文本工具必须用户点击执行后才运行。
- 文件工具步骤不从计划参数直接执行，仍需走 V6 文件工具选择框。
- 步骤输出不会自动发送给 AI。
- 用户点击“继续规划”才会把已查看的输出作为下一轮计划输入。
- 继续规划有轮次上限，避免无限循环。

## 5. 验证结果

已执行：

```powershell
cmake --build build-qt
ctest --test-dir build-qt --output-on-failure
git diff --check
```

结果：

- 构建通过。
- `ctest` 共 28 个测试，全部通过。
- `git diff --check` 通过。

## 6. 新增测试

V7 新增测试：

- `AgentToolCatalogTest`
- `AgentPlanParserTest`
- `AgentPlanPromptBuilderTest`
- `AgentPlanExecutorTest`
- `AgentPlanDialogSmokeTest`

覆盖点：

- 工具目录 ID 唯一。
- 工具风险等级有效。
- 合法计划解析。
- 非 JSON、缺字段、未知工具、步骤过多、非法风险等级拒绝。
- 工具目录风险等级覆盖 AI 低估风险。
- 计划 Prompt 不包含禁用工具。
- 低风险文本工具执行。
- 文件工具不能从计划预览直接执行。
- 计划窗口基础控件。
- 用户点击执行后才产生输出。
- 输出可插入聊天输入框。
- 用户确认后可继续规划。

## 7. 手工验证建议

合并前建议按 [V7 手工验证脚本](25-v7-manual-test-script.md) 检查，重点包括：

- 输入目标后点击“Agent 计划”，能发起计划请求。
- 真实 AI 返回 JSON 计划时，计划窗口正常打开。
- 真实 AI 返回 Markdown 代码块包裹 JSON 时可以正常解析。
- 真实 AI 返回非 JSON 或不合规 JSON 时，只显示解析失败提示，不影响普通聊天。
- 计划窗口能展示步骤、工具、风险、原因和参数。
- 低风险文本工具步骤执行成功。
- 文件工具步骤的执行按钮不可用，需要通过文件工具窗口处理。
- 输出可以复制。
- 输出可以插入聊天输入框，但不会自动发送。
- 点击“继续规划”会基于已查看输出生成下一轮计划。
- 继续规划达到上限后按钮不可用。

## 8. 风险和遗留项

当前仍保留的限制：

- 真实 AI 计划生成质量依赖模型是否严格按 JSON 输出。
- 计划预览 UI 仍是独立窗口，尚未做主窗口侧栏式 Agent 面板。
- 文件工具与计划窗口的联动仍偏保守，不能直接从计划参数执行文件读取。
- 多轮继续规划只完成基础入口，尚未实现完整执行历史和计划链路展示。
- 尚未做专门的真实 API 自动化测试。
- 暂不支持 Shell 命令、脚本执行、键鼠模拟或后台无人值守任务。

这些限制是有意保留的，目的是让 V7 先完成受控 Agent 雏形，不提前进入 V8 默认工作目录文件 Agent 范围。

## 9. 后续建议

V7 合并前优先按 [V7 手工验证脚本](25-v7-manual-test-script.md) 做真实 DeepSeek Key 手工验证。验证通过后，可以进入两个方向：

- V7 增强：计划历史、计划链路展示、文件工具步骤更自然地跳转到文件工具窗口。
- V8 准备：默认工作目录、工作目录文件工具、连续执行、停止入口和文件内容提示词注入防护。

V8 的自动文件生成和连续执行不应在 V7 手工验收前正式合并。
