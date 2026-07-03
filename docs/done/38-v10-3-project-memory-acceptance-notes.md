# V10.3 验收记录：受控工作记忆

## 1. 范围

V10.3 的目标是增加受控工作记忆，让 Agent 可以读取项目记忆，并在用户确认后追加新的项目记忆。

本阶段已完成：

- 新增 `ProjectMemoryService`。
- 从 Agent 项目目录读取 `AGENT_MEMORY.md`。
- Prompt 中明确标记记忆是受限、不可信项目上下文。
- 新增 `memory.append_project_note` 工具。
- 记忆追加必须通过计划窗口和用户确认。
- 记忆文件不存在时，追加工具会创建 `AGENT_MEMORY.md`。
- 记忆内容会拒绝明显敏感字段，例如 API Key、password、token、Bearer、secret。
- `ApplicationController` 在 Agent 计划请求和统一 Agent 入口中注入项目记忆。

## 2. 记忆文件

默认路径：

```text
<Agent 项目目录>/AGENT_MEMORY.md
```

追加后的文件示例：

```markdown
# Agent Memory

Only store information the user explicitly asked the assistant to remember.

## 2026-06-03T12:00:00Z
Source: user

Prefer running ctest before commit.
```

## 3. 关键文件

```text
src/tools/ProjectMemoryService.h
src/tools/ProjectMemoryService.cpp
src/tools/AgentToolRegistry.cpp
src/app/AgentPlanPromptBuilder.h
src/app/AgentPlanPromptBuilder.cpp
src/app/ApplicationController.cpp
tests/tools/ProjectMemoryServiceTest.cpp
tests/tools/AgentToolRegistryTest.cpp
tests/tools/AgentToolCatalogTest.cpp
tests/app/AgentPlanPromptBuilderTest.cpp
```

## 4. 安全边界

- 不自动保存聊天全文。
- 不自动记忆模型输出。
- 只有用户明确要求记住的内容才应使用 `memory.append_project_note`。
- 工具执行仍需要计划预览和用户确认。
- 明显包含凭据或密钥的内容会被拒绝。
- 记忆只作为项目上下文，不能覆盖系统安全规则、工具权限、工作目录限制或确认要求。
- 默认单条记忆最大 2000 字符。
- 默认读取记忆文件前 16 KB。

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
100% tests passed, 0 tests failed out of 40
```

新增或扩展的测试覆盖：

- `ProjectMemoryServiceTest`：验证缺失记忆、追加记忆、读取记忆、Prompt 包装、敏感内容拒绝和空内容拒绝。
- `AgentToolRegistryTest`：验证 `memory.append_project_note` schema 和实际执行。
- `AgentToolCatalogTest`：验证记忆工具进入工具目录。
- `AgentPlanPromptBuilderTest`：验证工作记忆片段注入统一 Prompt。

## 6. 当前限制

- 暂不支持记忆编辑、删除或 UI 管理。
- 暂不支持记忆条目结构化检索。
- 暂不支持按会话区分记忆。
- 记忆敏感内容检测是基础关键词版本，不能替代完整 DLP。

## 7. 后续建议

V10 主线第一版已经具备项目指令、外部技能和工作记忆。下一阶段建议进入 V11：

- 增加更多开发者工具，例如 `git.review_diff`、`logs.summarize`。
- 增加 tool result 回传后的自动重规划。
- 增加真实 AI 手工验证脚本。
