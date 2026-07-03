# V11 验收记录：工具生态扩展、统一模式与开发者工作流

## 1. 范围

V11 的目标是在 V1-V10.3 稳定基础上，扩展 6 个开发者工具、完善 Chat/Agent 统一模式切换、并进行完整验收。

本阶段已完成：

- 6 个 V11 工具的完整实现和注册（`git.review_diff` / `git.review_log` / `logs.summarize` / `data.csv_read` / `data.csv_write` / `project.find_files`）
- Chat/Agent 统一模式（`sendUnifiedMessage`），AI 自动判断聊天还是任务执行
- `UnifiedResponseParser` 解析 AI 响应，支持 chat/plan 两种 JSON 格式
- Native Function Calling 工具调用和 JSON plan fallback 双路径兼容
- 取消生成后占位消息正确更新为"已停止"
- 计划窗口正确弹出、聊天消息正确更新
- `ctest` 全量 46 个测试通过

## 2. V11 工具清单

### P0 工具（已完成）

| 工具 ID | 能力 | 风险 | 状态 |
|---------|------|------|------|
| `git.review_diff` | 分析 git diff 变更 | Low | ✅ 已实现 |
| `git.review_log` | 查看最近提交记录 | Low | ✅ 已实现 |
| `logs.summarize` | 搜索/过滤应用日志 | Low | ✅ 已实现 |

### P1 工具（已完成）

| 工具 ID | 能力 | 风险 | 状态 |
|---------|------|------|------|
| `data.csv_read` | 读取工作目录内 CSV | Low | ✅ 已实现 |
| `data.csv_write` | 写工作目录内 CSV | Medium | ✅ 已实现 |
| `project.find_files` | 按模式搜索项目文件 | Low | ✅ 已实现 |

### P2 工具（未纳入）

| 工具 ID | 能力 | 状态 |
|---------|------|------|
| `web.search` | 搜索公开资料 | 未纳入 V11 |
| `release.package` | 按白名单流程打包 | 未纳入 V11 |

## 3. 统一模式（Unified Agent）

### 触发路径

```
用户输入 → MainWindow → sendUnifiedMessage(content)
  → 构建 UnifiedPrompt（包含工具目录 + 聊天/计划双格式）
  → sendChatWithTools(config, requestSession, functionToolSchemas)
  → handleRequestFinished()
    ├── Native Tool Calls → AgentToolCallPlanBuilder::buildPlanFromToolCalls
    │   ├── 成功 → emit agentPlanReady → 弹出计划窗口
    │   └── 失败 → 显示错误消息在聊天区
    └── JSON 文本响应 → UnifiedResponseParser::parse
        ├── kind=chat → 纯聊天展示
        ├── kind=plan → AgentPlanParser::parseJsonPlan → 弹出计划窗口
        └── 解析失败 → 显示原始回复兜底
```

### 验证场景（静态代码审查）

| # | 场景 | 预期行为 | 代码路径 | 状态 |
|---|------|----------|----------|------|
| 1 | Agent 模式 AI 返回工具调用 | 弹出计划窗口 | `handleRequestFinished` → `agentPlanReady` | ✅ |
| 2 | Plan 解析成功 | 聊天消息显示"计划已生成"，计划窗口弹出 | `m_currentAssistantContent = "Agent 计划已生成..."` | ✅ |
| 3 | Plan 解析失败 | 聊天区显示错误提示，不弹出计划窗口 | `else { m_currentAssistantContent = "工具调用解析失败..." }` | ✅ |
| 4 | 取消生成 | 占位消息更新为"已停止"，状态清理 | `cancelCurrentRequest` → `"(已停止)"` | ✅ |

### Fallback 链

1. Native Function Calling 成功 → 直接转换 plan ✅
2. Native Function Calling 失败 → 降级为不带 tools 的 JSON plan 请求 ✅
3. JSON plan 解析失败 → 显示错误消息 ✅
4. 非 JSON 响应 → 当作纯聊天展示 ✅

## 4. 关键文件

```text
# V11 工具实现
src/tools/GitReviewService.h
src/tools/GitReviewService.cpp
src/tools/LogSummaryService.h
src/tools/LogSummaryService.cpp
src/tools/CsvDataService.h
src/tools/CsvDataService.cpp
src/tools/ProjectFindService.h
src/tools/ProjectFindService.cpp

# V11 工具注册
src/tools/AgentToolRegistry.cpp  (lines 951-1080)

# 统一模式
src/app/AgentPlanPromptBuilder.h  (UnifiedResponseKind, UnifiedResponse, UnifiedResponseParser)
src/app/AgentPlanPromptBuilder.cpp  (buildUnifiedPrompt, UnifiedResponseParser::parse)
src/app/ApplicationController.h  (sendUnifiedMessage, ActiveRequestKind::UnifiedAgent)
src/app/ApplicationController.cpp  (sendUnifiedMessage 实现)

# V11 测试
tests/tools/GitReviewServiceTest.cpp
tests/tools/LogSummaryServiceTest.cpp
tests/tools/CsvDataServiceTest.cpp
tests/tools/ProjectFindServiceTest.cpp
```

## 5. 安全边界

- `git.*` 工具：只执行只读命令（diff/log），禁止 add/commit/push
- `logs.*` 工具：脱敏 API Key/Token/密码；限制输出行数
- `data.*` 工具：限定工作目录内；CSV 行数上限（读取 500 行、写入 500 行）
- `project.*` 工具：不扫描 `.git`、`build-*`、`node_modules` 等目录

## 6. 测试覆盖率

| 测试 | 模块 | 状态 |
|------|------|------|
| GitReviewServiceTest | `git.review_diff` / `git.review_log` | ✅ 通过 |
| LogSummaryServiceTest | `logs.summarize` | ✅ 通过 |
| CsvDataServiceTest | `data.csv_read` / `data.csv_write` | ✅ 通过 |
| ProjectFindServiceTest | `project.find_files` | ✅ 通过 |
| AgentToolRegistryTest | 全量工具注册验证 | ✅ 通过 |
| AgentToolCatalogTest | 工具目录完整性 | ✅ 通过 |
| AgentPlanPromptBuilderTest | Prompt 生成 | ✅ 通过 |
| AgentPlanParserTest | JSON Plan 解析 | ✅ 通过 |
| AgentToolCallPlanBuilderTest | Function Calling → Plan | ✅ 通过 |
| AgentLoopControllerTest | Agentic Loop 执行 | ✅ 通过 |
| AgentPlanExecutorTest | 计划执行器 | ✅ 通过 |
| AgentPlanDialogSmokeTest | 计划窗口冒烟 | ✅ 通过 |

**总计：46/46 测试通过（100%）**

## 7. 已知限制

1. **`UnifiedResponseParser::parse` 无独立单元测试** — 统一响应的 chat/plan/fallback 三条路径依靠 `ApplicationController::handleRequestFinished` 间接验证，缺少独立的解析器单测。
2. **`sendUnifiedMessage` 全链路无自动化测试** — Agent 模式下消息发送到计划窗口弹出的完整流程仅通过静态代码审查和手工验证覆盖。
3. **统一模式不支持对话上下文** — `sendUnifiedMessage` 构建的 `requestSession` 是独立会话，不包含当前聊天的历史消息。
4. **OCR 工具为占位实现** — `system.ocr_text` 当前仅读文件返回占位文本，完整 OCR 需后续集成 Windows OCR API。
5. **V12/V13/V14 工具已注册但功能不完整** — `system.list_windows`、`system.capture_screen`、`input.click_button` 等工具已注册，但部分为占位实现。

## 8. 回归验证

在 Git 仓库中验证：

```bash
# 全量测试
cd build-qt && ctest --output-on-failure

# 结果：100% tests passed, 0 tests failed out of 46
```

## 9. 验收结论

- ✅ 6 个 V11 工具全部实现并注册到 `AgentToolRegistry`
- ✅ 每个工具有参数 schema、风险等级和测试
- ✅ Chat/Agent 统一模式实现，4 个场景代码逻辑正确
- ✅ Fallback 链完整：Native FC → JSON Plan → Chat
- ✅ 取消生成后状态清理正确
- ✅ `ctest` 46/46 通过
- ⚠️ `UnifiedResponseParser` 和 `sendUnifiedMessage` 缺独立测试
- ⚠️ 统一模式不支持多轮对话上下文

**整体评估：V11 通过验收。** 6 个工具生态扩展和统一模式核心功能完整，安全边界明确，自动化测试覆盖关键路径。已知限制不阻塞使用，建议在后续版本补全测试覆盖和对话上下文。
