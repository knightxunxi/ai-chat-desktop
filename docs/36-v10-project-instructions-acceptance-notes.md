# V10.1 验收记录：项目级指令文件

## 1. 范围

V10 的长期目标是项目级指令、外部技能系统和工作记忆。本记录覆盖第一切片：读取项目目录中的 `AGENT.md` 并注入 Agent 计划 Prompt。

本阶段已完成：

- 新增 `ProjectInstructionService`。
- 从设置中的 Agent 项目目录读取 `AGENT.md`。
- 缺少 `AGENT.md` 时静默跳过，不影响 Agent 计划生成。
- 对 `AGENT.md` 设置 16 KB 默认读取上限，超出时截断。
- Prompt 中明确标记 `AGENT.md` 是不可信项目数据，不能覆盖系统安全规则、工具注册表限制、工作目录/项目目录限制或用户确认要求。
- `AgentPlanPromptBuilder::buildPlanningPrompt` 支持可选项目指令块。
- `AgentPlanPromptBuilder::buildUnifiedPrompt` 支持可选项目指令块。
- `ApplicationController::generateAgentPlan` 和聊天内统一 Agent 入口已接入项目级指令读取。

## 2. 关键文件

```text
src/app/ProjectInstructionService.h
src/app/ProjectInstructionService.cpp
src/app/AgentPlanPromptBuilder.h
src/app/AgentPlanPromptBuilder.cpp
src/app/ApplicationController.cpp
tests/app/ProjectInstructionServiceTest.cpp
tests/app/AgentPlanPromptBuilderTest.cpp
```

## 3. 安全边界

- `AGENT.md` 只作为项目上下文，不是系统指令。
- `AGENT.md` 不能扩大工具权限。
- `AGENT.md` 不能绕过用户确认。
- `AGENT.md` 不能覆盖工作目录、项目目录、命令白名单或受保护文件策略。
- 内容过大时只读取前 16 KB，避免 Prompt 被过长项目文件挤满。

## 4. 验证结果

构建：

```powershell
cmake --build build-qt
```

结果：通过。

测试：

```powershell
ctest --test-dir build-qt --output-on-failure
```

结果：

```text
100% tests passed, 0 tests failed out of 38
```

新增或扩展的测试覆盖：

- `ProjectInstructionServiceTest`：验证缺失文件、正常读取、Prompt 安全包装、截断和非法参数。
- `AgentPlanPromptBuilderTest`：验证计划 Prompt 和统一 Prompt 能插入项目指令块。

## 5. 当前限制

- 只支持项目目录根部的 `AGENT.md`，暂不递归读取子目录指令。
- 暂不支持 UI 展示当前已加载的项目指令。
- 暂不支持外部技能文件。
- 暂不支持工作记忆。

## 6. 后续建议

V10.2 外部技能文件系统和 V10.3 受控工作记忆已完成第一版，分别见 [V10.2 验收记录](37-v10-2-external-skills-acceptance-notes.md) 和 [V10.3 验收记录](38-v10-3-project-memory-acceptance-notes.md)。后续建议进入 V11：

- 增加工具生态扩展。
- 增加开发者工作流工具。
- 增加项目指令、外部技能和工作记忆手工验证脚本。
