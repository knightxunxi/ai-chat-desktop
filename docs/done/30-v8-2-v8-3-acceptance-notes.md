# V8.2/V8.3 验收记录：Agentic Loop 与工具注册表

## 1. 版本目标

V8.2 的目标是把 Agent 从“一次性计划执行”推进到“观察 -> 选择下一步 -> 执行 -> 评估”的循环运行模式。

V8.3 的目标是把工具描述、参数 schema 和执行逻辑收敛到统一的工具注册表，并预留 Function Calling 兼容层。

本阶段仍然不包含：

- 任意 PowerShell/CMD 命令执行。
- 键盘鼠标模拟。
- 浏览器自动化。
- 后台无人值守长期任务。

## 2. V8.2 完成内容

已完成：

- 新增 `AgentLoopController`。
- 计划窗口连续执行改为使用 `AgentLoopController`。
- 每轮循环执行一个工具步骤。
- 增加最大连续步数，默认 5 步。
- 增加运行总耗时上限和单步耗时上限。
- 支持停止请求，停止后不再执行后续步骤。
- 工具失败后暂停。
- 工具失败后保留失败摘要，可作为后续重新规划输入。
- 增加重复动作检测。
- 增加 Agent Loop 审计日志，记录 Observe、Think、Act、Evaluate。
- 新增 `AgentLoopPromptBuilder`，用于生成单步动作规划提示词。
- 新增 `AgentLoopActionParser`，支持解析 `done=true` 或单步 `step` JSON。

## 3. V8.3 完成内容

已完成：

- 新增 `AgentToolRegistry`。
- 新增 `AgentToolDefinition`。
- 每个工具统一包含：
  - `toolId`
  - 描述信息
  - 风险等级
  - 参数 schema
  - Function Calling 函数名
  - 是否允许计划窗口直接执行
  - 执行函数
- `AgentToolCatalog` 改为从注册表生成。
- `AgentPlanExecutor` 改为通过注册表执行工具。
- 生成 OpenAI-compatible `tools` schema。
- Function Calling 函数名会把工具 ID 中的点号转换为下划线，例如 `workspace.write_text` -> `workspace_write_text`。
- JSON plan fallback 保留，现有 `AgentPlanParser` 仍可继续使用。

## 4. 安全边界

Agent Loop 边界：

- 每轮只执行一个步骤。
- 达到步数上限会暂停。
- 用户停止后不再执行后续步骤。
- 工具失败后不继续盲目执行。
- 重复动作会被拦截。
- 审计日志只记录工具 ID、状态、耗时和输出长度，不记录敏感正文。

工具注册表边界：

- 工具目录和执行逻辑来自同一份注册表。
- 文件选择类工具仍不能从计划窗口直接执行。
- Function Calling schema 只导出可直接执行的文本工具和工作目录工具。
- 工作目录工具仍经过 `WorkspacePolicy` 校验。

## 5. 验证结果

构建命令：

```powershell
cmake -S . -B build-qt -G "MinGW Makefiles" -DCMAKE_C_COMPILER=D:/QT/Tools/mingw1310_64/bin/gcc.exe -DCMAKE_CXX_COMPILER=D:/QT/Tools/mingw1310_64/bin/g++.exe
cmake --build build-qt
```

测试命令：

```powershell
ctest --test-dir build-qt --output-on-failure
```

结果：

```text
100% tests passed, 0 tests failed out of 33
```

## 6. 新增测试覆盖

新增测试：

- `AgentLoopActionParserTest`
- `AgentLoopPromptBuilderTest`
- `AgentLoopControllerTest`
- `AgentToolRegistryTest`

覆盖点：

- 单步 action JSON 解析。
- `done=true` 终止信号解析。
- 工具风险等级由本地目录提升。
- 单步规划 prompt 包含不可信观察边界。
- Agent Loop 完成、停止、失败、步数上限和重复动作检测。
- 注册表工具 ID 唯一。
- Function Calling 函数名唯一且不含点号。
- Function Calling schema 包含参数 schema。
- 文本工具和工作目录工具可通过注册表执行。
- 文件选择类工具不会进入 Function Calling schema。

## 7. 当前限制

- `AgentLoopController` 第一版仍是同步执行，长时间工具只能在步骤结束后检测超时。
- 真实 AI 单步循环请求尚未替代现有计划生成入口。
- Function Calling schema 已生成；原生 tools 请求和 tool_calls 计划转换已在 V9.2 第一版接入。
- 命令执行仍未开放，需等 V9 白名单命令策略。

## 8. 后续建议

下一阶段建议进入 V9：

- 新增 `CommandPolicy`。
- 新增 `CommandRunner`。
- 只允许白名单命令。
- 固定工作目录。
- 增加命令超时和输出摘要。
- 命令输出作为不可信数据回传。
