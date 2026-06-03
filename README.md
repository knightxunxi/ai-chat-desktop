# AI Chat Desktop

一个基于 C++、Qt 6 和 CMake 的桌面 AI 聊天应用。用户可以配置 DeepSeek 或其他 OpenAI 兼容接口的 API Key、Base URL 和模型名称，然后进行多轮对话。

本项目同时作为软件开发流程学习项目，包含需求说明、技术方案、任务拆分、验收记录、Windows 打包说明和阶段复盘。

## 功能特性

- OpenAI 兼容聊天接口。
- DeepSeek 和 OpenAI 服务商预设。
- API Key、Base URL、模型名称和可手动输入的可选模型参数配置。
- API Key 使用 Windows Credential Manager 保存。
- 中文/英文界面语言切换。
- 当前会话多轮上下文。
- 当前会话角色提示词。
- AI 回复流式展示。
- 本地保存配置。
- SQLite 保存聊天记录。
- 多会话列表、新建、切换、重命名、搜索、导出和删除。
- 停止生成。
- 消息复制。
- 当前会话导出为 Markdown。
- AI 消息基础 Markdown 渲染。
- AI 代码块独立复制。
- 角色提示词模板和 JSON 导入导出。
- 错误分类提示和失败重试。
- 基础日志记录、应用内日志查看和打开日志目录。
- 受控文件工具：读取用户选择的文本文件、列出文件夹、保存工具输出、打开确认后的文件或文件夹。
- Agent 结构化计划：AI 生成工具步骤，用户在计划窗口确认后执行。
- Agent 工作目录文件工具：在默认工作目录内创建、读取、列目录、覆盖和移动删除普通文件。
- Agent 连续执行 MVP：计划窗口可连续执行最多 5 个可直接执行步骤，并支持停止。
- Agentic Loop 运行层：观察、执行、评估循环，支持步数上限、停止、失败暂停和重复动作检测。
- 工具注册表和 Function Calling 兼容层：统一工具描述、参数 schema、执行入口和函数调用 schema。
- 原生 Function Calling 第一版：Agent 请求可声明 tools，并将模型返回的 tool_calls 转换为计划预览。
- 受控命令执行 MVP：通过白名单模板执行 Git 状态检查、diff 检查、构建、测试和项目文件列表。
- 开发者命令技能：内置检查改动、提交前检查、构建并测试和定位测试失败等命令流程。
- 项目级指令文件：Agent 请求会读取项目目录下的 `AGENT.md` 作为受限项目上下文。
- 外部技能文件：Agent 请求会读取项目目录下的 `skills/*.skill.md` 并注入推荐技能流程。
- 受控工作记忆：Agent 可读取 `AGENT_MEMORY.md`，并可在用户确认后追加项目记忆。
- Chat/Agent 统一模式切换：AI 自动判断聊天还是任务执行，计划窗口自动弹出。
- 6 个开发者工具生态：Git 变更审查、Git 提交记录、日志摘要、CSV 读写和项目文件搜索。
- Windows Release 打包说明。

## 技术栈

- C++17
- Qt 6 Widgets
- Qt Network
- Qt SQL / SQLite
- CMake
- MinGW

当前开发环境使用：

```text
Qt: 6.10.2
Compiler: Qt bundled MinGW 13.1.0
CMake: 4.3.0-rc1
Platform: Windows
```

注意：Qt 与 MinGW 版本需要匹配。若使用非 Qt 套件自带的 MinGW，可能出现链接错误。

## 项目结构

```text
.
├─ docs/                 项目文档
├─ learn/                架构学习、技术讲解和面试复盘资料
├─ resources/            Qt 资源和样式
├─ src/
│  ├─ app/               应用控制层
│  ├─ core/              核心数据模型
│  ├─ services/          AI API 客户端和流式解析
│  ├─ support/           日志等通用支持模块
│  ├─ storage/           配置和聊天记录存储
│  ├─ tools/             本地工具和受控文件交互
│  └─ ui/                Qt Widgets 界面
├─ tests/                自动化测试
└─ CMakeLists.txt
```

## 文档索引

- [需求说明](docs/01-requirements.md)
- [技术方案](docs/02-technical-design.md)
- [任务拆分](docs/03-task-breakdown.md)
- [V1 验收记录](docs/04-v1-acceptance-notes.md)
- [Windows 打包说明](docs/05-windows-packaging.md)
- [V1 复盘与 V2 规划](docs/06-v1-retrospective.md)
- [V2 Roadmap](docs/07-v2-roadmap.md)
- [V2 验收与复盘](docs/08-v2-acceptance-notes.md)
- [V3 Roadmap](docs/09-v3-roadmap.md)
- [V3 API Key 安全存储方案](docs/10-v3-credential-storage-design.md)
- [V3 验收与复盘](docs/11-v3-acceptance-notes.md)
- [V3 Release Notes](docs/12-v3-release-notes.md)
- [V4 Roadmap](docs/13-v4-roadmap.md)
- [代码格式规范](docs/14-code-style.md)
- [小工具集成方案](docs/15-tool-integration-design.md)
- [V5 Roadmap](docs/16-v5-roadmap.md)
- [会话组织增强设计](docs/17-session-organization-design.md)
- [V5 验收记录](docs/18-v5-acceptance-notes.md)
- [V6 Roadmap](docs/19-v6-roadmap.md)
- [V6 本地交互安全设计](docs/20-v6-local-interaction-security.md)
- [V6 验收记录](docs/21-v6-acceptance-notes.md)
- [Agent 与电脑交互后续规划](docs/22-agent-automation-roadmap.md)
- [V7 Roadmap](docs/19-v7-roadmap.md)
- [V7 Agent 安全设计](docs/23-v7-agent-safety-design.md)
- [V7 验收记录](docs/24-v7-acceptance-notes.md)
- [V7 手工验证脚本](docs/25-v7-manual-test-script.md)
- [V8 Agent 工作目录规划](docs/26-v8-agent-workspace-roadmap.md)
- [下一版本开发计划](docs/27-next-version-development-plan.md)
- [V8+ Agent 详细开发路线](docs/28-v8-plus-agent-development-roadmap.md)
- [V8.1 验收记录](docs/29-v8-acceptance-notes.md)
- [V8.2/V8.3 验收记录](docs/30-v8-2-v8-3-acceptance-notes.md)
- [V9+ 后续开发规划](docs/31-v9-plus-development-roadmap.md)
- [V9 命令执行安全设计](docs/32-v9-command-execution-security.md)
- [V9 验收记录](docs/33-v9-command-execution-acceptance-notes.md)
- [V9.1 验收记录](docs/34-v9-1-command-skills-acceptance-notes.md)
- [V9.2 验收记录](docs/35-v9-2-function-calling-acceptance-notes.md)
- [V10.1 验收记录](docs/36-v10-project-instructions-acceptance-notes.md)
- [V10.2 验收记录](docs/37-v10-2-external-skills-acceptance-notes.md)
- [V10.3 验收记录](docs/38-v10-3-project-memory-acceptance-notes.md)
- [V11+ 后续开发规划](docs/39-v11-plus-development-roadmap.md)
- [V11 工具生态设计](docs/40-v11-tool-ecosystem-design.md)
- [V12 电脑感知设计](docs/41-v12-computer-awareness-design.md)
- [V14 个人管家整合设计](docs/42-v14-assistant-integration-design.md)
- [V11 验收记录](docs/43-v11-acceptance-notes.md)
- [V12-V15 重新规划（对标分析后）](docs/44-v12-v15-roadmap.md)
- [优化方向（含主流 Agent 对标）](docs/优化方向.md)
- [学习资料入口](learn/README.md)

## 构建方式

在项目根目录执行：

```powershell
qt-cmake -S . -B build-qt -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=D:\QT\Tools\mingw1310_64\bin\g++.exe
cmake --build build-qt
```

如果 `qt-cmake` 不在 `PATH` 中，可以使用完整路径：

```powershell
& "D:\QT\6.10.2\mingw_64\bin\qt-cmake.bat" -S . -B build-qt -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=D:\QT\Tools\mingw1310_64\bin\g++.exe
cmake --build build-qt
```

## 运行方式

开发环境下运行：

```powershell
$env:PATH="D:\QT\6.10.2\mingw_64\bin;D:\QT\Tools\mingw1310_64\bin;$env:PATH"
.\build-qt\AIChatDesktop.exe
```

首次使用建议打开设置窗口，填写：

```text
Base URL: https://api.deepseek.com
Model: deepseek-v4-flash
API Key: 你的 API Key
```

API Key 只保存在本机配置中，不应写入源码或提交到 Git。

## 日志

应用会在本机应用数据目录写入基础日志，用于定位 API 请求开始、完成、取消和失败原因。日志不记录 API Key、请求体或聊天内容。

Windows 上通常位于：

```text
%APPDATA%\AIChatDesktop\AI Chat Desktop\ai-chat-desktop.log
```

## 测试

```powershell
ctest --test-dir build-qt --output-on-failure
```

## CI

项目使用 GitHub Actions 执行基础 CI：

- pull request 自动触发。
- 推送到 `main` 自动触发。
- Windows Debug 和 Release 构建。
- 构建完成后运行 CTest。

当前测试覆盖：

- 核心模型。
- SSE 流式响应解析。
- OpenAI 兼容请求体构建。
- 模型参数请求体构建。
- HTTP 错误分类映射。
- Windows 凭据存储抽象和旧配置迁移。
- SQLite 聊天记录存储。
- 设置窗口基础行为。
- 消息复制和 Markdown 展示行为。
- 当前会话 Markdown 导出。
- 样式资源加载。
- 基础日志写入和敏感字段脱敏。
- 日志最近内容读取。
- 角色提示词模板保存和选择。
- 本地工具逻辑和工具窗口 smoke test。
- 受控文件交互服务和文件工具窗口 smoke test。
- 会话收藏、归档和筛选。
- Agent 工具目录、计划解析、计划 Prompt 生成、计划预览和低风险步骤执行。
- Agent 默认工作目录配置和工作目录策略。
- Agent 工作目录文件服务、`workspace.*` 工具执行、连续执行和提示词注入防护。
- Agentic Loop 单步 action 解析、单步 prompt、循环控制器和工具注册表。
- 受控命令执行策略、命令运行器、命令工具注册和命令执行边界。
- 开发者命令技能目录和项目目录配置化。
- 应用启动/关闭 smoke test。
- V11 工具测试：Git 审查、日志摘要、CSV 读写、项目文件搜索。测试总数：46 个（100% 通过）。
- V12 工具测试：窗口枚举、屏幕截图。

## Windows 打包

Release 构建和 `windeployqt` 部署步骤见：

[Windows 打包说明](docs/05-windows-packaging.md)

简要流程：

```powershell
cmake -S . -B build-release-qt -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=D:\QT\Tools\mingw1310_64\bin\g++.exe
cmake --build build-release-qt
New-Item -ItemType Directory -Force release\AIChatDesktop
Copy-Item build-release-qt\AIChatDesktop.exe release\AIChatDesktop\
windeployqt release\AIChatDesktop\AIChatDesktop.exe
```

发布前请在 `release\AIChatDesktop\AIChatDesktop.exe` 中手工验证 API 请求、语言切换、聊天记录恢复、停止生成、消息复制、Markdown 展示、角色提示词模板、错误重试、应用内日志查看、本地工具窗口、文件工具窗口、收藏/归档和会话筛选。

## 当前版本状态

当前版本已完成 V11 第一版，覆盖本地工具系统、受控文件交互工具、Agent 结构化计划、计划预览、默认 Agent 工作目录、`workspace.*` 文件工具、Agentic Loop 运行层、工具注册表、Function Calling schema、原生 tool_calls 计划转换、白名单命令执行、项目目录配置化、开发者命令技能目录、`AGENT.md` 项目级指令、外部技能文件、受控工作记忆、Chat/Agent 统一模式切换和 6 个开发者工具生态扩展（Git 审查、日志摘要、CSV 读写、项目文件搜索）。

已知限制：

- 当前版本只面向 Windows，暂未规划 macOS/Linux 迁移。
- Markdown 展示仍是基础版本，复杂代码高亮暂未支持；V5 新增的 Markdown 整理工具只做低风险空白清理。
- 角色提示词模板仍是本地版本，暂未支持云同步。
- 本地工具系统仅支持内置工具，暂不支持插件、脚本或任意命令执行。
- Agent 只允许在配置的工作目录内自动操作普通文件，不支持工作目录外自动读写。
- 连续执行是 MVP，同步步骤会在当前步骤完成后响应停止。
- 原生 Function Calling 已完成第一版，但真实接口稳定性和 tool result 回传重规划仍需后续验证。
- 命令执行只支持固定白名单模板，不支持任意 PowerShell/CMD，也不支持 `git add`、`git commit`、`git push`。
- 开发者命令技能已支持外部技能文件，但暂未支持 UI 一键触发或热重载提示。
- 项目级指令只读取项目目录根部的 `AGENT.md`，暂不支持子目录指令或 UI 展示。
- 工作记忆暂不支持 UI 管理、结构化检索、编辑或删除。
- V11 统一模式暂不支持多轮对话上下文（独立会话发送）。
- `UnifiedResponseParser` 缺独立单元测试；统一模式全链路无自动化测试。
- OCR 工具（`system.ocr_text`）为占位实现，完整 OCR 需后续集成 Windows OCR API。

## 后续方向

V11 已完成后，下一步建议进入 V12 电脑感知（窗口枚举、截图、OCR），详见 [V12 电脑感知设计](docs/41-v12-computer-awareness-design.md)。中长期方向见 [V11+ 后续开发规划](docs/39-v11-plus-development-roadmap.md)。
