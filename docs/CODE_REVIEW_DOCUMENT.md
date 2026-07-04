# CodeXX 项目代码审查与功能定位文档

> 创建日期：2026-07-04
> 审查范围：src/ + tests/，约 31,000 行 C++，14 个 CMake 子库，69 测试
> 目的：记录每个功能的实现位置和关键接口，便于后续开发快速定位

---

## 一、项目架构概览

```
src/
├── core/        → 数据模型层（AppConfig / ChatSession / AgentPlan）
├── support/     → 基础设施层（AppLogger / LogFileReader）
├── storage/     → 持久化层（SQLite / 凭据 / 配置）
├── services/    → 网络服务层（AI 客户端 / Python sidecar / 流式解析）
├── app/         → 逻辑编排层（Controller / Orchestrator / Coordinators）
├── tools/       → 工具系统层（7 个 CMake 子库，60+ 工具）
│   ├── registry/  → 工具注册表与执行器
│   ├── core/      → 文件操作、命令执行
│   ├── perception/→ 截图/OCR/窗口枚举
│   ├── input/     → 鼠标/键盘输入模拟
│   ├── dev/       → Git/CSV/日志搜索
│   ├── text/      → JSON/Markdown/文本处理
│   └── assistant/ → 工作日报/系统信息
├── memory/      → 记忆系统层（L1-L3 记忆 / 日期日志）
├── skills/      → 技能系统层（SKILL.md 解析 / 匹配）
├── hooks/       → 钩子系统层（6 个生命周期钩子）
├── scheduler/   → 定时任务层（Cron 调度 / JSON 持久化）
├── mcp/         → MCP 协议层（JSON-RPC over QProcess / TLS）
└── ui/          → 界面展示层（MainWindow / ChatView / 各种对话框）
```

---

## 二、功能 → 代码位置映射

### 2.1 聊天基础

| 功能 | 主文件 | 关键类/函数 | 说明 |
|------|-------|-------------|------|
| 流式聊天 | `src/services/OpenAICompatibleClient.h/.cpp` | `sendChat()` → `StreamParser` | SSE 流式 HTTP 请求 |
| SSE 解析 | `src/services/StreamParser.h/.cpp` | `parseChunk()` / `aggregateJson()` | 增量 JSON 合并 |
| 多会话管理 | `src/storage/ChatHistoryStorage.h/.cpp` | `loadSessions()` / `saveSession()` | SQLite CRUD |
| API Key 安全存储 | `src/storage/CredentialStorage.h/.cpp` | `WindowsCredentialStorage` | Win32 Credential Manager |
| 配置管理 | `src/storage/ConfigStorage.h/.cpp` | `load()` / `save()` | JSON 文件 |
| 角色提示词 | `src/storage/PromptTemplateStorage.h/.cpp` | `importFromJson()` / `mergeDuplicates()` | 模板管理 |
| 会话收藏/归档 | `src/app/SessionCoordinator.h/.cpp` | `setFavorite()` / `setArchived()` + `SessionListFilter` | 三态筛选 |
| Markdown 导出 | `src/storage/ChatSessionExporter.h/.cpp` | `exportToMarkdown()` | 聊天记录 → .md |
| 日志系统 | `src/support/AppLogger.h/.cpp` | `info()` / `warning()` / `error()` + LogFileReader | 轮转日志 |
| 错误分类 | `src/services/RequestErrorCategory.h` | `RequestErrorCategory` 枚举 | 三级错误分类 |
| 消息模型 | `src/core/ChatMessage.h` / `ChatSession.h` | `ChatMessage` / `ChatSession` 结构体 | 核心数据模型 |

### 2.2 Agent 系统

| 功能 | 主文件 | 关键类/函数 | 说明 |
|------|-------|-------------|------|
| Agent 循环编排 | `src/app/AgentOrchestrator.h/.cpp` | `executeAgentLoopIteration()` / `executePlanAndReportToChat()` | OODA 循环，最多 50 轮 |
| 异步循环引擎 | `src/app/AgentLoopController.h/.cpp` | `AgentLoopEngine` (QObject) / `executeLoop()` | QTimer 驱动，非阻塞 |
| 上下文窗口 | `src/app/ContextWindowManager.h/.cpp` | `checkAndCompressIfNeeded()` | 85% 阈值触发压缩 |
| AI 客户端抽象 | `src/services/AIClient.h` | `AIClient` 虚基类 | 统一接口 |
| 工具调用兼容层 | `src/services/ToolCall.h` | `ToolCallList` / `parseToolCalls()` | Function Calling 适配 |
| 计划解析 | `src/app/AgentPlanParser.h/.cpp` | `parsePlan()` / `PlanStep` | AI 结构化计划提取 |
| 计划执行器 | `src/app/AgentPlanExecutor.h/.cpp` | `executeStep()` | 单步执行 |
| 提示词生成 | `src/app/AgentLoopPromptBuilder.h/.cpp` | `buildNextActionPrompt()` / `classifyGoal()` | 7 种意图分类 |
| 技能匹配 | `src/app/AgentLoopPromptBuilder.cpp` | `matchSkills()` | 子串匹配 |
| 流式工具结果 | `src/app/AgentOrchestrator.cpp` | `addPendingToolResult()` / `takePendingToolResults()` | 边生成边执行 |
| 对话分叉 | `src/app/SessionCoordinator.h/.cpp` | `cycleBranch()` / `createMessageBranch()` | 分支消息 |
| 消息编辑+重试 | `src/app/SessionCoordinator.cpp` | `truncateCurrentSessionFrom()` / `editCurrentMessage()` | 截断重发 |
| 重复动作检测 | `src/app/AgentOrchestrator.cpp` | `m_actionFingerprints` (参数键级指纹) | 防死循环 |
| AutoFix 闭环 | `src/app/AgentOrchestrator.cpp` (line 532+) | AutoFix 检测编辑→构建→测试→注入 | 自动修复 |
| 工具序列记忆 | `src/app/AgentLoopPromptBuilder.cpp` | `recordToolSequence()` | 成功序列持久化 |

### 2.3 工具系统 (60+ 工具)

| 类别 | 注册位置 | 服务文件 | 工具数 |
|------|---------|---------|:------:|
| 文件操作 | `AgentToolRegistry.cpp` (line 754+) | `FileInteractionService`, `WorkspaceFileService` | 15 |
| 命令执行 | `AgentToolRegistry.cpp` (line 1015+) | `CommandRunner` | 6 |
| 桌面感知 | `AgentToolRegistry.cpp` (line 1262+) | `ScreenCaptureService`, `OcrService`, `WindowDetector` | 9 |
| 键鼠输入 | `AgentToolRegistry.cpp` (line 1480+) | `InputSimulator`, `ForegroundValidator` | 8 |
| 网络请求 | `AgentToolRegistry.cpp` (line 1600+) | 内联 QNetworkAccessManager | 4 |
| 代码运行 | `AgentToolRegistry.cpp` (line 1855+) | QProcess + 临时文件 | 2 |
| 开发工具 | `AgentToolRegistry.cpp` (Memory & Git) | `ProjectFindService`, `GitReviewService` | 6 |
| 文本处理 | `AgentToolRegistry.cpp` (line 710+) | `JsonFormatTool`, `MarkdownCleanupTool` | 4 |
| 助手工具 | `AgentToolRegistry.cpp` (line 2023+) | `AssistantService` | 4 |
| MCP 工具 | `src/mcp/McpRegistry.cpp` | `McpRegistry::callTool()` | 动态 |

### 2.4 UI 组件

| 组件 | 文件 | 关键函数 |
|------|------|---------|
| 主窗口 | `src/ui/MainWindow.h/.cpp` (1700+ 行) | `setupUi()` / `sendCurrentMessage()` |
| 聊天视图 | `src/ui/ChatView.h/.cpp` (345 行) | `addMessage()` / `updateLastAssistantMessage()` |
| 消息气泡 | `src/ui/MessageWidget.h/.cpp` | `setContent()` / `markdownStyleSheet()` |
| 代码高亮 | `src/ui/CodeHighlighter.h/.cpp` | 12 种语言语法着色 |
| Token 条 | `src/ui/TokenBar.h/.cpp` | 绿/橙/红三色进度条 |
| 打字指示器 | `src/ui/TypingIndicator.h/.cpp` | 400ms 三点动画 |
| 命令面板 | `src/ui/CommandPalette.h/.cpp` | Ctrl+Shift+P 模糊搜索 |
| 步骤分组 | `src/ui/AgentStepGroupWidget.h/.cpp` | 折叠摘要卡片 |
| 设置窗口 | `src/ui/SettingsDialog.h/.cpp` | 多厂商/API 配置 |
| 角色编辑 | `src/ui/RolePromptDialog.h/.cpp` | 系统提示词编辑 |
| 日志查看器 | `src/ui/LogViewerDialog.h/.cpp` | 日志搜索过滤 |
| 统计面板 | `src/ui/StatisticsDialog.h/.cpp` | 使用数据统计 |
| 计划预览 | `src/ui/AgentPlanDialog.h/.cpp` | 结构化计划展示 |
| 确认弹窗 | `src/ui/ConfirmToolDialog.h/.cpp` | 高风险工具确认 |
| 文件工具 | `src/ui/FileToolsDialog.h/.cpp` | 文件操作面板 |
| 工具窗口 | `src/ui/ToolsDialog.h/.cpp` | 工具列表 |

### 2.5 Python 能力层侧车

| 能力 | Python 文件 | 方法 |
|------|------------|------|
| 服务入口 | `python/agent_sidecar/agent_sidecar/server.py` | `main()` / JSONL 循环 |
| 协议 | `python/agent_sidecar/agent_sidecar/protocol.py` | `handle_line()` / `_dispatch()` |
| 能力集 | `python/agent_sidecar/agent_sidecar/capabilities.py` | `ping()` / `chat()` / `count_tokens()` |
| 多厂商管理 | `python/agent_sidecar/agent_sidecar/providers.py` | `load_providers()` / `resolve_provider()` |
| 厂商配置 | `python/agent_sidecar/agent_sidecar/providers.json` | 3 个默认厂商 |
| C++ 客户端 | `src/services/PythonSidecarClient.h/.cpp` | `send()` / `listProviders()` |
| C++ AI 封装 | `src/services/PythonSidecarAIClient.h/.cpp` | 实现 AIClient 接口 |

### 2.6 记忆系统

| 层级 | 文件 | 关键函数 |
|------|------|---------|
| L1 记忆 | `src/memory/ProjectMemoryManager.h/.cpp` | `appendDailyLog()` / ISO 周压缩 |
| L2 记忆 | `src/memory/ProjectMemoryManager.cpp` | `readMemory()` / `writeMemory()` |
| L3 记忆 (Skills) | `src/skills/SkillManager.h/.cpp` | `matchSkills()` / 双目录扫描 |
| 每日日志 | `src/memory/DailyMemoryWriter.h/.cpp` | 按日期文件追加 |

### 2.7 Skills + Hooks + MCP + 调度

| 系统 | 文件 | 说明 |
|------|------|------|
| Skill 管理 | `src/skills/SkillManager.h/.cpp` | SKILL.md 双目录 + 优先级排序 |
| Skill 解析 | `src/skills/SkillFileParser.h/.cpp` | YAML frontmatter 解析 |
| Hook 管理 | `src/hooks/HookManager.h/.cpp` | 6 个生命周期点 |
| 内置 Hook | `src/hooks/BuiltinHooks.h/.cpp` | 权限校验 / 危险命令 |
| MCP Registry | `src/mcp/McpRegistry.h/.cpp` | 服务器注册 • 路由 • config 序列化 |
| MCP 连接器 | `src/mcp/McpConnector.h/.cpp` | QProcess / QSslSocket 双模式 • TLS |
| 定时任务 | `src/scheduler/TaskScheduler.h/.cpp` | Cron 表达式调度 |
| 任务存储 | `src/scheduler/TaskStorage.h/.cpp` | JSON 文件持久化 |

---

## 三、设计决策要点

| 决策 | 实现位置 | 核心逻辑 |
|------|---------|---------|
| ApplicationController 为 Facade | `src/app/ApplicationController.h` | 聚合 3 个 Coordinator |
| 工具必须通过注册表校验 | `src/tools/Ag entToolRegistry.cpp` | `makeDefinition` 统一 Schema |
| QEventLoop 同步等待 | `AgentLoopController.cpp` (executeLoop) | 测试兼容，主流程已异步化 |
| QProcess 封装而非 std::system | `CommandRunner.cpp` | 分离通道 • 超时 • 黑名单 |
| 记忆用 Markdown 而非 SQLite | `ProjectMemoryManager.cpp` | 人机可读 • 低心智负担 |
| 手写 assert 而非 QTest | `tests/` 各文件 | 零依赖 • 简洁 |
| SendInput 而非 Qt Test | `InputSimulator.cpp` | Win32 真实输入模拟 |
| SKILL.md 单文件格式 | `SkillFileParser.cpp` | YAML frontmatter + Markdown |

---

## 四、测试结构 (69 个)

| 测试范围 | 文件 | 测试数 |
|---------|------|:------:|
| 配置 | `tests/core/ConfigTest.cpp` | 2 |
| 信���体 | `tests/services/MessageBuilderTest.cpp` | 3 |
| 流式解析 | `tests/services/StreamParserTest.cpp` | 4 |
| 数据库存储 | `tests/storage/ChatHistoryStorageTest.cpp` | 3 |
| 凭据存储 | `tests/storage/CredentialStorageTest.cpp` | 2 |
| Token 估算 | `tests/app/TokenEstimatorTest.cpp` | 3 |
| 设置窗口 | `tests/ui/SettingsDialogTest.cpp` | 5 |
| 角色窗口 | `tests/ui/RolePromptDialogTest.cpp` | 3 |
| Agent 循环 | `tests/app/AgentLoopControllerTest.cpp` | 10 |
| Agent 执行 | `tests/app/AgentLoopExecutionTest.cpp` | 2 |
| 计划解析 | `tests/app/AgentPlanParserTest.cpp` | 4 |
| Mock 集成 | `tests/services/AgentE2ETest.cpp` | 5 |
| MCP | `tests/mcp/McpConnectorTest.cpp` + `McpRegistryTest.cpp` | 3 |
| 调度 | `tests/app/TaskSchedulerTest.cpp` | 4 |
| 消息 Widget | `tests/ui/MessageWidgetTest.cpp` | 3 |
| 工具目录 | `tests/tools/AgentToolCatalogTest.cpp` | 3 |
| 工具注册 | `tests/tools/FrontToolRegistryTest.cpp` | 5 |
| 控制层 | `tests/app/ControllerMessagesTest.cpp` | +4 |
| UI 冒烟 | `tests/ui/MainWindowUITest.cpp` + `ChatViewTest.cpp` | +5 |

---

## 五、CI / 部署

| 项目 | 位置 | 说明 |
|------|------|------|
| GitHub Actions | `.github/workflows/ci.yml` | MSVC 编译+测试+Release 打包 |
| 打包脚本 | `ci/package.ps1` | windeployqt + zip |
| Release 构建 | `build-release-qt/` | MinGW Release 2.6 MB |
| Python sidecar | `python/agent_sidecar/` | 20 测试，可选依赖 |

---

## 六、已知技术债务

| 问题 | 位置 | 说明 |
|------|------|------|
| MainWindow ~1700 行 | `src/ui/MainWindow.cpp` | 可拆分为 ViewManager |
| AC 行数偏多 | `src/app/ApplicationController.cpp` | StreamCoordinator 已拆分，仍有优化空间 |
| QEventLoop 遗留 | `AgentLoopController.cpp` (executeLoop) | 同步包装，仅测试使用 |
| 硬编码 Markdown 颜色 | `MessageWidget.cpp` | 已支持双主题，性能优化可选 |
| API Key 日志暴露 | `AppLogger.cpp` | 已增加 sanitization |
| 无玩偶��试 | 暂无 | 所有感知/输入抗风险 |

---

> 后续开发：按照 `AGENT.md` → `docs/DOCUMENT_INDEX.md` → `docs/DEVELOPMENT_WORKFLOW.md` → MCP 查询 → 本文档定位代码 → 开发 → 验证 → 同步文档 的流程。
