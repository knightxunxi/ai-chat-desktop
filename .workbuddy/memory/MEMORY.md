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

## 架构总结
- **3 种请求模式**: ChatMessage（纯聊天）/ AgentPlan（废弃入口）/ UnifiedAgent（统一模式）
- **API 端点**: `sendMessage` (Chat) | `sendMessageWithTools` (Chat+工具) | `sendAgentLoopMessage` (Agent循环)
- **权限**: requiresUserConfirmation=true 在 Chat 无高权限时跳过；Agent 模式全放行
- **MinGW 修复**: WIN32_EXECUTABLE FALSE + -mwindows 链接标志（__imp___argc 问题）

## 已知问题
- 项目有 4 个 exe 构建目录 (build/, build-qt/, build-release-qt/, release/)，需保持同步
