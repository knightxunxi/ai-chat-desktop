# V10.2 验收记录：外部技能文件系统

## 1. 范围

V10.2 的目标是把 V9.1 的静态开发者命令技能扩展为可从项目目录读取的外部技能文件。

本阶段已完成：

- 新增 `AgentCommandSkillFileService`。
- 从 Agent 项目目录读取 `skills/*.skill.md`。
- 技能文件使用 Markdown 包裹 JSON，或者直接写 JSON。
- 外部技能会追加到内置技能后面。
- 外部技能重复 ID 不覆盖内置技能。
- 技能步骤中的工具必须存在于 `AgentToolRegistry`，并且允许计划窗口直接执行。
- 技能步骤风险等级会按本地工具目录提升，不能低于工具本身风险。
- `AgentPlanPromptBuilder` 支持传入合并后的技能 Prompt 片段。
- `ApplicationController` 在 Agent 计划请求和统一 Agent 入口中注入内置技能加外部技能。

## 2. 技能文件格式

建议路径：

```text
<Agent 项目目录>/skills/pre-commit-check.skill.md
```

格式示例：

```json
{
  "id": "project.pre_commit_check",
  "englishName": "Project Pre-Commit Check",
  "chineseName": "项目提交前检查",
  "englishDescription": "Run the project-specific pre-commit checks.",
  "chineseDescription": "运行项目自定义提交前检查。",
  "steps": [
    {
      "englishTitle": "Check Git status",
      "chineseTitle": "检查 Git 状态",
      "toolId": "command.git_status",
      "englishReason": "Inspect changed files before running checks.",
      "chineseReason": "运行检查前先查看改动文件。",
      "risk": "low"
    },
    {
      "englishTitle": "Run tests",
      "chineseTitle": "运行测试",
      "toolId": "command.ctest",
      "englishReason": "Verify the automated test suite.",
      "chineseReason": "验证自动化测试集。",
      "risk": "medium"
    }
  ]
}
```

## 3. 关键文件

```text
src/app/AgentCommandSkillFileService.h
src/app/AgentCommandSkillFileService.cpp
src/app/AgentCommandSkillCatalog.h
src/app/AgentCommandSkillCatalog.cpp
src/app/AgentPlanPromptBuilder.h
src/app/AgentPlanPromptBuilder.cpp
src/app/ApplicationController.cpp
tests/app/AgentCommandSkillFileServiceTest.cpp
tests/app/AgentCommandSkillCatalogTest.cpp
tests/app/AgentPlanPromptBuilderTest.cpp
```

## 4. 安全边界

- 技能文件只描述推荐步骤，不直接执行工具。
- 技能文件不能新增工具能力。
- 技能文件不能绕过 `AgentToolRegistry`。
- 技能文件不能覆盖内置技能。
- 技能中的工具仍需要用户在计划窗口确认后才执行。
- 命令类步骤仍受 `CommandPolicy` 白名单限制。
- 单个技能文件默认最大 32 KB，单个项目默认最多读取 20 个技能文件。

## 5. 验证结果

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
100% tests passed, 0 tests failed out of 39
```

新增或扩展的测试覆盖：

- `AgentCommandSkillFileServiceTest`：验证缺失目录、有效技能、未知工具、重复 ID 和非法项目目录。
- `AgentCommandSkillCatalogTest`：验证指定技能列表可以独立生成 Prompt。
- `AgentPlanPromptBuilderTest`：验证外部技能 Prompt 片段可以注入统一 Agent Prompt。

## 6. 当前限制

- 外部技能暂不在 UI 中单独展示。
- 外部技能暂不支持热重载提示。
- 外部技能暂不支持参数模板，当前步骤参数仍为空对象。
- 外部技能暂不支持多目录层级，只读取项目目录根部 `skills` 文件夹。

## 7. 后续建议

V10.3 受控工作记忆已完成第一版，见 [V10.3 验收记录](38-v10-3-project-memory-acceptance-notes.md)。后续建议进入 V11。

建议优先实现：

- 只保存用户明确要求记录的信息。
- 记忆文件落在项目目录或应用数据目录的受控路径。
- Prompt 注入时标注记忆来源和时间。
- 不保存 API Key、Token、密码或聊天全文。
