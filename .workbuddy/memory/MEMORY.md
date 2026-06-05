# CodeXX 项目记忆

## 项目概要
- CodeXX: 基于 Qt6/C++ 的 AI 编程助手桌面应用
- 技术栈: Qt6 Widgets, C++17, CMake, OpenAI-compatible API
- 构建系统: CMake + MinGW (实测 g++)
- 测试框架: 自定义轻量级测试 (非 QtTest/GoogleTest)

## V12 基础设施升级 (2026-06-03 完成)
- V12.1: ContextWindowManager 上下文窗口管理（三入口接入）
- V12.2: AgenticLoopController 无限循环（去掉硬编码 maxIterations=2）
- V12.3: 流式工具执行（StreamParser → OpenAICompatibleClient → ApplicationController 全链路）
- V12.4: Chat 模式自动执行工具（⚡自动执行按钮 + 高权限复选框 + 权限分级）
- V12.5: Agent 模式自动执行 + 移除 AgentPlanDialog 信号链（计划自动执行结果整合到聊天）
- V12.6: Agent 连续循环执行（分析→执行→判断→继续，AgentLoopPromptBuilder，上限50轮）
- 测试: 52/52 全部通过（预存 AppLaunchSmokeTest MinGW 链接已修复）
- AgentPlanDialog.cpp/.h 已从 CMakeLists.txt 移除，物理文件仍保留

## V13.1 三层记忆系统 (2026-06-03 完成)
- 新增 src/memory/ 模块：MemoryEntry, DailyMemoryWriter, ProjectMemoryManager
- L1 ~/.codex/MEMORY.md → L2 .workbuddy/memory/MEMORY.md → L3 YYYY-MM-DD.md
- Agent 执行完成后自动追加每日日志；循环中注入记忆到 systemPrompt
- 敏感内容检测 (api_key/token/password/secret/bearer/credential/private_key)，超1000字符截断
- 测试: 53/53 全部通过

## V13.2 L3参数调整 + 记忆压缩 (2026-06-03 完成)
- 参数: 单条截断 2000 字符, 14 天窗口, 50 条上限, 30K 总预算硬上限
- 压缩接口: buildCompressionPrompts (ISO周分组 → LLM提示词) + applyCompression (落盘 YYYY-Www-compressed.md)
- buildMemorySection 自动注入压缩摘要 → systemPrompt
- 测试: 53/53 全部通过 (ProjectMemoryManagerTest: 19 断言)

## V13.3 Skills + Hooks 系统 (2026-06-04 完成)
- 新增 src/skills/ 模块：SkillDefinition, SkillFileParser (手写YAML), SkillManager (双目录+子串匹配+优先级合并)
- 新增 src/hooks/ 模块：HookManager (6个Hook点), BuiltinHooks (Timestamp/RateLimit/SensitiveFilter), ScriptHookRunner (QProcess沙箱)
- 集成: executeLoop()/buildNextActionPrompt()/AgentToolRegistry::execute() 嵌入 Hook 调用点
- ApplicationController 持有 SkillManager+HookManager，循环完成后 emit 技能摘要信号
- 测试: 55/55 全部通过 (SkillManagerTest 11 + HookManagerTest 11)

## 架构总结
- **3 种请求模式**: ChatMessage（纯聊天）/ AgentPlan（废弃入口）/ UnifiedAgent（统一模式）
- **API 端点**: `sendMessage` (Chat) | `sendMessageWithTools` (Chat+工具) | `sendAgentLoopMessage` (Agent循环)
- **权限**: requiresUserConfirmation=true 在 Chat 无高权限时跳过；Agent 模式全放行
- **MinGW 修复**: WIN32_EXECUTABLE FALSE + -mwindows 链接标志（__imp___argc 问题）

## V14.2 操作层重写 + V14.3 闭环 (2026-06-04 完成)
- UiAutomationService.cpp: 占位 → Win32 真实实现（GetForegroundWindow + 系统窗口黑名单保护）
- InputSimulator.cpp: 占位 → SendInput(KEYEVENTF_UNICODE) 真实键盘输入
- AgentLoopPromptBuilder.cpp: perception 引导后追加 action tools 引导（input.validate_foreground/click_button/type_text）
- 安全链路: 操作前 → 系统保护窗口检查 → ForegroundValidator
- 新增 InputSimulatorTest.cpp（6 条断言：空文本/有效文本/空组合键/Ctrl+C/Alt+F4/Foo+Bar）
- 测试: 57/57 全部通过（56 现有 + 1 新增），零回归

## V15 ApplicationController 重构 (2026-06-04 完成)

### 背景
ApplicationController.h/cpp 随功能迭代持续膨胀（~350行/.h, ~1600行/.cpp），职责混杂：配置管理、会话持久化、Agent 循环编排、Skills/Hooks 等全部耦合在一个类中。

### 拆分结果 (T02: R1.1~R1.6, 全由 software-engineer-3 实施)

| 文件 | 重构前 | 重构后 | 变化 |
|------|--------|--------|------|
| ApplicationController.h | ~350 行 | 220 行 | -37% |
| ApplicationController.cpp | ~1600 行 | 1270 行 | -21% |
| ConfigCoordinator.h/cpp (新建) | 0 | 96 行 | 配置+Prompt模板 |
| SessionCoordinator.h/cpp (新建) | 0 | 514 行 | 会话生命周期+持久化 |
| AgentOrchestrator.h/cpp (新建) | 0 | 632 行 | Agent循环+Skills/Hooks/MCP |

- 测试: **56/63 通过**（7 个预存失败，零回归）
- 构建: 零编译错误
- 新文件: ConfigCoordinator, SessionCoordinator, AgentOrchestrator（均在 `src/app/`）

### 架构
AC 保留：AI 客户端（m_aiClient）、Chat 消息流（sendMessage→handleTextDelta→handleRequestFinished）、3 个 Coordinator 成员。Agent 路径通过 AgentOrchestrator 编排。Coordinator signals → AC connect 转发 → MainWindow（MainWindow 不直接依赖 Coordinator 类型）。

### T04: UI 测试补充 (2026-06-05 完成)
- 新增 **ChatViewTest** (16 用例): 构造/增删/搜索/流式更新/TokenBar/DebugCard/AgentStep
- 新增 **MainWindowSmokeTest** (10 用例): 构造/核心控件/按钮/窗口属性/析构
- 测试: **58/65 通过**（7 个预存失败，零回归），新增 26 条断言全部通过
- 发现: ChatView::messageCount() 计数所有布局条目（含 DebugCard/AgentStep），非仅 MessageWidget

## V15.4 T03 tools/ 子库化 (2026-06-04 完成)
- 7 个子库: registry, core, perception, input, dev, text, assistant
- umbrella CMakeLists.txt 挂 7 子库 + AgentToolRegistry.cpp
- 修复 3 个构建问题：循环依赖 + 2 个缺少 codexx_tools_core 链接
- 测试: 58/65, 零回归

## V15.5 内置 Skill 注册 (2026-06-05 完成)
- 修复 SkillManager::scanDirectory: entryInfoList → QDirIterator::Subdirectories（支持子目录 SKILL.md）
- 修复预存失败 SkillManagerTest::TC-10
- 创建 8 个 SKILL.md 在 .workbuddy/skills/：4 个文本工具 + 4 个文件工具
- 全量: 59/65 (+1), 零回归

## DeepSeek API 多模态格式
- DeepSeek **不支持** OpenAI 的 `image_url` 格式
- 正确格式: `{"type": "image", "image": {"data": "<raw_base64>", "format": "base64"}}`
- base64 数据需剥离 `data:image/png;base64,` 前缀
- 参考: OpenClaw 社区踩坑记录 + CSDN 技术博客

## 已知问题
- 项目有 4 个 exe 构建目录 (build/, build-qt/, build-release-qt/, release/)，需保持同步
