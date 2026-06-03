# V9+ 后续开发规划：受控命令执行、技能记忆与电脑感知

本文档基于当前代码、README、`learn/` 架构说明、V7/V8 安全设计、V8+ 路线和本地优化方向整理。它用于替代“只看总览”的规划方式，作为 V9 及以后阶段的执行级参考。

当前参考关系：

- 总体路线仍以 [V8+ Agent 详细开发路线](28-v8-plus-agent-development-roadmap.md) 为主线。
- 本文档作为 V9 之后的详细开发计划。
- UI 大改、跨平台迁移暂不纳入本路线；当前项目继续按 Windows 桌面应用推进。

## 1. 当前项目状态判断

当前已完成或基本具备：

- C++17、Qt 6 Widgets、CMake、SQLite、本地配置和 Windows Credential Manager。
- 聊天、多会话、角色提示词、日志、导出、基础 Markdown 展示。
- V6 本地工具和受控文件交互。
- V7 Agent 结构化计划、计划解析、工具目录、计划预览和单步执行。
- V8.1 默认工作目录文件 Agent，支持 `workspace.*` 文件工具。
- V8.2 Agentic Loop 第一版，支持步数上限、停止、失败暂停和重复动作检测。
- V8.3 `AgentToolRegistry`，统一工具描述、参数 schema、执行入口和 Function Calling schema 生成。
- V9 命令执行 MVP 第一版，支持固定白名单命令模板、命令策略、命令运行器、输出脱敏和命令工具注册，见 [V9 验收记录](33-v9-command-execution-acceptance-notes.md)。
- V9.1 开发者命令技能与项目目录配置化已完成第一版，见 [V9.1 验收记录](34-v9-1-command-skills-acceptance-notes.md)。
- V9.2 原生 Function Calling 接入已完成第一版，见 [V9.2 验收记录](35-v9-2-function-calling-acceptance-notes.md)。
- V10.1 项目级指令文件已完成第一版，见 [V10.1 验收记录](36-v10-project-instructions-acceptance-notes.md)。
- V10.2 外部技能文件系统已完成第一版，见 [V10.2 验收记录](37-v10-2-external-skills-acceptance-notes.md)。
- V10.3 受控工作记忆已完成第一版，见 [V10.3 验收记录](38-v10-3-project-memory-acceptance-notes.md)。

当前代码中还出现了一个值得单独收尾的方向：

- `ApplicationController::sendUnifiedMessage`
- `AgentPlanPromptBuilder::buildUnifiedPrompt`
- `UnifiedResponseParser`
- `MainWindow` 的 Chat/Agent 模式切换按钮

这说明项目已经开始从“独立 Agent 计划按钮”过渡到“聊天内 Agent 统一入口”。该方向建议作为 V8.4 收尾，再进入 V9 命令执行。

## 2. 当前缺口

进入 V9 前需要明确这些缺口：

- 原生 Function Calling 已完成第一版，但还缺 tool result 回传、多轮自动重规划和真实接口稳定性验证。
- `AgentLoopController` 当前仍主要执行已有计划中的下一步，真实 AI 单步循环尚未完全替代整批计划生成。
- 命令执行模块已完成第一版，但项目目录仍来自应用启动时的当前工作目录，后续应配置化。
- 当前工具注册表适合扩展，但命令类工具需要更严格的参数模板和风险分级。
- 聊天内 Agent 入口已有雏形，但需要补测试、文档和失败兜底。

## 3. 版本路线总览

推荐后续版本关系：

```text
V8.4：聊天内 Agent 统一入口收尾
-> V9：受控命令执行 MVP
-> V9.1：构建/测试/Git 开发者命令技能
-> V9.2：原生 Function Calling 接入
-> V10：项目级指令、技能系统和工作记忆
-> V11：工具生态扩展和开发者工作流
-> V12：电脑感知、操作记录和确认回放
-> V13：受控设备输入模拟
-> V14：个人 AI 管家能力整合
```

优先级判断：

| 阶段 | 优先级 | 说明 |
| --- | --- | --- |
| V8.4 | P0 | 先稳定当前已出现的统一入口改动 |
| V9 | P0 | 已完成 MVP 第一版 |
| V9.1 | P1 | 已完成第一版 |
| V9.2 | P1 | 已完成第一版 |
| V10 | P1 | 已完成项目级指令、外部技能和工作记忆第一版 |
| V11 | P2 | 下一阶段，扩展更多工具但不破坏安全边界 |
| V12 | P3 | 进入电脑感知和操作记录 |
| V13 | P3 | 设备输入模拟，风险高，必须后置 |
| V14 | P3 | 长期整合方向 |

## 4. V8.4：聊天内 Agent 统一入口收尾

目标：

- 让用户可以在聊天输入框中切换 Chat/Agent 模式。
- Agent 模式下，AI 自行判断是普通聊天还是生成工具计划。
- 保留独立 `AgentPlanDialog` 作为执行确认和结果展示窗口。

建议任务：

1. 稳定 `UnifiedResponseParser`。
2. 明确统一响应 JSON schema。
3. 补充 `AgentPlanPromptBuilderTest` 对统一 Prompt 的覆盖。
4. 补充统一模式解析测试。
5. 补充 MainWindow smoke test，验证模式切换按钮和发送路由。
6. 更新 README 和 learn，说明 Chat/Agent 模式边界。
7. 手工验证 DeepSeek 下普通聊天和任务计划两种输出。

验收标准：

- Chat 模式保持普通聊天行为。
- Agent 模式下，普通问答可以显示为正常助手消息。
- Agent 模式下，文件生成、文本工具等任务可以打开计划窗口。
- 统一响应解析失败时，会回退成普通聊天展示，不导致窗口卡死。
- 取消生成能清理 Agent 响应缓存。

## 5. V9：受控命令执行 MVP

目标：

- 让 Agent 能执行有限、白名单、可审计、可超时的命令。
- 第一版只服务于项目开发场景：查看 Git 状态、检查 diff、构建、运行测试、列目录。

V9 不做：

- 任意 PowerShell/CMD 字符串执行。
- 删除、格式化、注册表修改、系统服务修改。
- 静默安装软件。
- 自动输入密码、API Key、Token。
- 键盘鼠标模拟。
- 工作目录外长期后台任务。

## 6. V9 设计原则

### 6.1 不走 shell 字符串

命令执行必须使用“程序 + 参数数组”的方式，例如：

```text
program: git
args: ["status", "--short", "--branch"]
cwd: projectRoot
```

禁止形式：

```text
git status && del file
powershell -Command "..."
cmd /c "..."
```

### 6.2 白名单命令模板

第一版建议只允许这些模板：

| 工具 ID | 程序 | 参数 | 风险 |
| --- | --- | --- | --- |
| `command.git_status` | `git` | `status --short --branch` | low |
| `command.git_diff_check` | `git` | `diff --check` | low |
| `command.git_diff_stat` | `git` | `diff --stat` | low |
| `command.cmake_build` | `cmake` | `--build build-qt` | medium |
| `command.ctest` | `ctest` | `--test-dir build-qt --output-on-failure` | medium |
| `command.list_project_files` | internal/Qt | list project directory | low |

说明：

- `list_project_files` 可优先复用现有文件服务，不一定真正启动命令。
- `cmake_build` 和 `ctest` 可能耗时，必须有超时和停止策略。
- 暂不开放 `git add`、`git commit`、`git push`，避免 Agent 直接影响版本库。

### 6.3 工作目录限制

命令执行默认工作目录：

```text
项目根目录 D:\C1\CodeXX
```

限制：

- 只能在项目根目录或 Agent 工作目录内执行。
- 禁止把 cwd 设置为系统目录、用户主目录根部、磁盘根目录。
- 命令输出中检测到疑似密钥、Token、Bearer 字段时需要脱敏。

### 6.4 输出处理

命令结果结构建议：

```json
{
  "exitCode": 0,
  "timedOut": false,
  "stdoutSummary": "...",
  "stderrSummary": "...",
  "stdoutLength": 1200,
  "stderrLength": 0
}
```

日志只记录：

- 工具 ID。
- cwd 摘要。
- 命令模板 ID。
- 退出码。
- 是否超时。
- 输出长度。
- 错误摘要。

日志禁止记录：

- 完整 stdout/stderr 长文本。
- API Key、Token、密码。
- 文件正文。
- 敏感完整路径。

## 7. V9 任务拆分

### V9-TASK-001 命令安全设计文档

范围：

- 新增 `docs/32-v9-command-execution-security.md`。
- 定义命令白名单、禁止命令、cwd 策略、输出脱敏和测试范围。

验收：

- 每个允许命令都有明确目的和风险等级。
- 禁止范围写清楚。
- 文档和实际实现保持一致。

当前状态：已完成第一版，见 [V9 命令执行安全设计](32-v9-command-execution-security.md)。

### V9-TASK-002 CommandPolicy

建议文件：

```text
src/tools/CommandPolicy.h
src/tools/CommandPolicy.cpp
tests/tools/CommandPolicyTest.cpp
```

职责：

- 根据工具 ID 找到命令模板。
- 校验 cwd。
- 禁止 shell 元字符和任意命令拼接。
- 限制超时时间。
- 标记风险等级。

验收：

- 白名单命令通过。
- 未知命令失败。
- `powershell`、`cmd`、`del`、`Remove-Item` 等失败。
- cwd 指向工作目录外危险位置失败。

当前状态：已完成第一版，自动化测试覆盖核心策略。

### V9-TASK-003 CommandRunner

建议文件：

```text
src/tools/CommandRunner.h
src/tools/CommandRunner.cpp
tests/tools/CommandRunnerTest.cpp
```

职责：

- 使用 `QProcess` 执行程序和参数数组。
- 设置工作目录。
- 设置超时。
- 读取 stdout/stderr。
- 对输出做长度限制和脱敏。
- 返回结构化 `ToolResult`。

验收：

- 正常命令返回成功。
- 非零退出码返回失败并包含摘要。
- 超时命令能终止。
- 输出过长时截断。
- 日志不记录完整输出。

当前状态：已完成第一版，使用 `QProcess` 执行程序和参数数组。

### V9-TASK-004 命令工具注册

范围：

- 将 `command.*` 工具加入 `AgentToolRegistry`。
- 为每个命令提供参数 schema。
- 标记哪些命令可由计划窗口直接执行。

验收：

- 命令工具 ID 唯一。
- 命令工具能进入 Agent Prompt。
- 非白名单命令不会进入 Function Calling schema。
- `AgentPlanExecutor` 可通过注册表执行命令工具。

当前状态：已完成第一版，`command.*` 工具已进入 `AgentToolRegistry`。

### V9-TASK-005 计划窗口命令展示

范围：

- 计划详情展示命令模板 ID、cwd、风险等级。
- 执行前对 medium 风险命令给出更明确提示。
- 执行完成后展示摘要，不展示无限长原始输出。

验收：

- 用户能知道即将执行什么。
- 用户能看到退出码和摘要。
- 停止按钮能阻止后续命令。

当前状态：已完成基础展示，计划详情会展示项目目录；medium 风险提示后续可继续增强。

### V9-TASK-006 构建和测试工具闭环

范围：

- 让 Agent 可建议并执行：
  - `command.git_status`
  - `command.git_diff_check`
  - `command.cmake_build`
  - `command.ctest`

验收：

- Agent 能生成“构建并运行测试”的计划。
- 本地能执行构建/测试命令。
- 构建失败时输出摘要可用于后续分析。
- 命令输出作为不可信数据，不改变工具权限。

当前状态：命令模板已可执行；真实 AI 生成计划还需要后续手工验证和技能封装。

### V9-TASK-007 V9 验收与文档

范围：

- 新增 V9 验收记录。
- 更新 README、learn、面试问答。
- 补充手工验证脚本。

验收：

- `cmake --build build-qt` 通过。
- `ctest --test-dir build-qt --output-on-failure` 通过。
- V9 文档和实际能力一致。

当前状态：V9 MVP 当时 `35/35` 自动化测试通过，见 [V9 验收记录](33-v9-command-execution-acceptance-notes.md)；V9.1 扩展后当时为 `36/36`；V9.2 扩展后当时为 `37/37`；V10.1 扩展后当时为 `38/38`；V10.2 扩展后当时为 `39/39`；V10.3 扩展后当前为 `40/40` 自动化测试通过。

## 8. V9.1：开发者命令技能

目标：

- 把白名单命令组合成稳定工作流，而不是让用户每次从零描述。
- 当前第一版已完成，见 [V9.1 验收记录](34-v9-1-command-skills-acceptance-notes.md)。

建议技能：

| 技能 | 组成步骤 |
| --- | --- |
| `检查当前改动` | `git_status` -> `git_diff_stat` -> 总结 |
| `提交前检查` | `git_diff_check` -> `cmake_build` -> `ctest` |
| `定位测试失败` | `ctest` -> 摘要失败测试 -> 建议查看相关文件 |
| `V9 验收` | 构建 -> 测试 -> diff check -> 文档检查 |

建议文件：

```text
skills/build-and-test.skill.md
skills/pre-commit-check.skill.md
skills/debug-test-failure.skill.md
```

验收标准：

- Agent 能列出可用技能。
- 技能能展开为工具步骤。
- 技能执行仍经过工具注册表和命令策略。
- 技能文档不包含密钥或本地私有信息。

当前状态：静态技能目录、技能 Prompt 注入、技能计划展开和项目目录配置化已完成；外部技能文件和 UI 触发入口尚未完成。

## 9. V9.2：原生 Function Calling 接入

目标：

- 从“AI 输出 JSON 文本，本地解析”升级为“请求体声明 tools，模型返回 tool calls”。
- 保留 JSON plan fallback，兼容 DeepSeek 或其他 OpenAI-compatible 服务商差异。

建议任务：

1. 给 `OpenAICompatibleClient::buildRequestBody` 增加可选 tools 参数。
2. 定义 `ToolCall` 数据结构。
3. 扩展 `StreamParser` 或响应解析逻辑，识别 tool call delta。
4. 增加 `AIClient` 信号：`toolCallReceived` 或等价事件。
5. ApplicationController 根据工具调用生成 `AgentPlanStep`。
6. 对不支持 tools 的模型保留 JSON plan。

验收标准：

- 支持 tools 的模型可以返回原生工具调用。
- 不支持 tools 的模型仍可走 JSON plan。
- 工具调用函数名可映射回工具 ID。
- 工具参数仍由本地 schema 和策略校验。

当前状态：已完成第一版，见 [V9.2 验收记录](35-v9-2-function-calling-acceptance-notes.md)。后续真实 tool result 回传和自动重规划放到 V10/V11 之后继续完善。

## 10. V10：项目级指令、技能系统与工作记忆

目标：

- 让 Agent 理解项目约定，复用常用流程，并能保存用户明确要求记录的信息。

### 10.1 项目级指令

建议文件：

```text
AGENT.md
```

内容：

- 项目技术栈。
- 构建命令。
- 测试命令。
- 禁止修改的目录。
- Git 工作流。
- 当前阶段路线。

当前状态：已完成第一版，见 [V10.1 验收记录](36-v10-project-instructions-acceptance-notes.md)。

### 10.2 技能注册系统

建议模块：

```text
src/skills/
```

职责：

- 读取技能文件。
- 校验技能需要的工具是否存在。
- 把技能展开为 Agent 计划模板。

当前状态：已完成第一版，见 [V10.2 验收记录](37-v10-2-external-skills-acceptance-notes.md)。

### 10.3 工作记忆

建议模块：

```text
src/memory/
```

记录内容：

- 用户明确要求保存的偏好。
- 已验证的构建/测试流程。
- 项目级决策。
- 常用工作目录。

当前状态：已完成第一版，见 [V10.3 验收记录](38-v10-3-project-memory-acceptance-notes.md)。

限制：

- 不自动保存 API Key、Token、密码。
- 不把聊天全文默认写入记忆。
- 记忆来源必须标注为用户明确、项目文档或工具结果摘要。

## 11. V11：工具生态和开发者工作流扩展

目标：

- 在命令执行和技能系统稳定后，扩展更多有实际价值的工具。

候选工具：

| 工具 | 用途 | 优先级 |
| --- | --- | --- |
| `git.review_diff` | 总结当前 diff 风险 | P1 |
| `logs.summarize` | 总结日志最近内容 | P1 |
| `data.csv_read` | 读取工作目录内 CSV | P2 |
| `data.csv_write` | 写工作目录内 CSV | P2 |
| `release.package` | 按白名单流程打包 | P2 |
| `web.search` | 查询公开资料并给出来源 | P3 |

说明：

- 每个工具必须进入 `AgentToolRegistry`。
- 每个工具必须有参数 schema、风险等级、测试和日志策略。
- 外部网络搜索需要明确来源，不应直接把网页内容当作系统指令。

## 12. V12：电脑感知、操作记录和确认回放

目标：

- 先让应用“看见”和“记录”操作，再考虑模拟输入。

建议能力：

- 枚举窗口。
- 截图保存到 Agent 工作目录。
- OCR 识别截图文本。
- 记录应用内工具调用链。
- 记录命令执行链。
- 用户确认后回放低风险步骤。

暂不做：

- 自动输入密码。
- 浏览器自动登录。
- 后台偷偷操作窗口。
- 对敏感窗口截图并写日志。

验收标准：

- 能看到当前前台窗口标题。
- 能保存截图到工作目录。
- 操作记录不包含敏感正文。
- 回放前展示步骤列表，失败立即暂停。

## 13. V13：受控设备输入模拟

目标：

- 在明确授权下，对测试窗口或明确目标窗口执行有限输入。

建议技术：

- Windows UI Automation：优先通过控件定位。
- SendInput：作为低级 fallback。
- 前台窗口校验。
- 停止按钮。
- 审计日志。

禁止：

- 自动输入密码、API Key、验证码。
- 绕过 UAC。
- 跨应用记录键盘输入。
- 对未知窗口执行点击。
- 坐标盲点点击作为主要策略。

验收标准：

- 只在测试窗口中验证。
- 能输入普通文本。
- 能点击明确控件。
- 停止按钮可中止后续动作。

## 14. V14：个人 AI 管家能力整合

目标：

- 把聊天、角色、文件、命令、技能、记忆和电脑感知整合成可持续使用的个人助手。

候选场景：

- 项目检查。
- 文件整理。
- 工作日报。
- 本地资料摘要。
- 长期角色提示词和角色记忆。
- 简单定时提醒。

暂不纳入：

- 游戏逆向。
- DLL 注入。
- 内存修改。
- 绕过反作弊。

## 15. 推荐下一步

当前 V9.1 第一版已完成。更稳的下一步是：

```text
1. 提交当前 V9/V9.1/V9.2 改动
2. 进入 V11 工具生态和开发者工作流扩展
3. 增加 `git.review_diff`
4. 增加 `logs.summarize`
5. 设计 tool result 回传后的自动重规划
6. 补真实 AI 手工验证脚本
```

原因：

- 命令执行、技能流程和 Function Calling 接入已具备第一版。
- V10 可以让 Agent 更理解项目约定，并复用可配置技能。
- JSON plan fallback 仍需要保留，避免兼容接口不稳定时阻塞功能。

## 16. 简历表达阶段

V9 完成后可以写：

> 在 C++/Qt 桌面 AI 应用中实现受控命令执行能力，支持 Agent 通过白名单模板执行 Git 状态检查、构建和测试命令，并通过工作目录限制、参数数组执行、超时控制、输出摘要和审计日志降低系统操作风险。

V10 完成后可以写：

> 设计项目级指令、技能注册和工作记忆机制，使 Agent 能复用项目构建测试流程、记录用户明确偏好，并在本地策略校验下组合多工具完成开发任务。

V12/V13 完成后再考虑写：

> 探索 Windows 桌面感知与受控输入自动化，通过窗口检测、截图/OCR、UI Automation、执行前确认和停止机制实现有限的人机交互模拟。

注意不要写：

- AI 可以任意操作电脑。
- AI 可以自动执行任意命令。
- AI 可以绕过系统权限。

更准确的表达是：

- 受控 Agent。
- 白名单命令执行。
- 本地策略校验。
- 工作目录隔离。
- 用户可停止和可审计。
