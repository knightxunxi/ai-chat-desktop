# V9.2 验收记录：原生 Function Calling 接入

## 1. 范围

V9.2 的目标是把 Agent 计划生成从“只依赖模型输出 JSON 文本”推进到“请求体声明 `tools`，模型可返回原生 `tool_calls`”。

本阶段已完成：

- `OpenAICompatibleClient::buildRequestBody` 支持可选 `tools` 参数。
- 带 tools 请求会写入 `tools` 和 `tool_choice=auto`。
- 新增 `ToolCall` 数据结构。
- `StreamParser` 支持解析流式 `delta.tool_calls`，并拼接分段返回的 `function.arguments`。
- `AIClient` 新增 `toolCallsReceived` 信号。
- 新增 `AgentToolCallPlanBuilder`，把 Function Calling 函数名映射回 `AgentToolRegistry` 工具 ID。
- `ApplicationController::generateAgentPlan` 和统一 Agent 入口优先发送带 tools 的请求。
- 如果服务商因模型或参数不兼容拒绝 tools 请求，会自动退回旧 JSON plan 请求一次。
- 如果模型没有返回 tool calls，仍按原 JSON plan fallback 解析。

## 2. 关键文件

```text
src/services/ToolCall.h
src/services/AIClient.h
src/services/OpenAICompatibleClient.h
src/services/OpenAICompatibleClient.cpp
src/services/StreamParser.h
src/services/StreamParser.cpp
src/app/AgentToolCallPlanBuilder.h
src/app/AgentToolCallPlanBuilder.cpp
src/app/ApplicationController.h
src/app/ApplicationController.cpp
tests/app/AgentToolCallPlanBuilderTest.cpp
tests/services/OpenAICompatibleClientTest.cpp
tests/services/StreamParserTest.cpp
```

## 3. 安全边界

- AI 返回的 `tool_calls` 仍然不是可信执行权限。
- 函数名必须能映射回 `AgentToolRegistry` 中已注册、启用且允许计划窗口直接执行的工具。
- 参数必须是 JSON object。
- 工具风险等级仍以本地工具目录为准。
- 真正执行工具时仍经过计划预览、用户确认、工具注册表和本地策略。
- 命令类工具仍只允许 `CommandPolicy` 中的白名单模板。

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
100% tests passed, 0 tests failed out of 37
```

新增或扩展的测试覆盖：

- `OpenAICompatibleClientTest`：验证请求体可携带 `tools` 和 `tool_choice=auto`。
- `StreamParserTest`：验证流式 `tool_calls` 参数分片可以拼接为完整 JSON 参数。
- `AgentToolCallPlanBuilderTest`：验证函数名映射、参数解析、风险等级、重复 ID 修正、未知函数和非法参数失败。

## 5. 当前限制

- 真实 DeepSeek/OpenAI-compatible 接口的 tool call 稳定性还需要手工验证。
- 当前第一版只把 tool calls 转换为计划预览，不自动静默执行。
- 还没有实现 tool result 回传给模型后的多轮自动重规划。
- `tool_choice` 第一版固定为 `auto`，暂未提供 UI 配置。

## 6. 后续建议

V10.1 项目级指令文件已完成第一版，见 [V10.1 验收记录](36-v10-project-instructions-acceptance-notes.md)。后续建议继续 V10：

- 把静态开发者技能升级为可读取的外部技能文件。
- 增加受控工作记忆，只保存用户明确要求记录的偏好和项目决策。
- 为 tool call 执行结果回传和自动重规划做设计。
