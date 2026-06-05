# CodeXX

CodeXX 是一个基于 Qt6/C++ 开发的 Windows 桌面 AI 编程助手。项目将流式对话、Agent 自动执行循环、本地工具调用、记忆系统、Skills、Hooks、MCP 外部工具接入和定时任务整合到一个原生 Widgets 应用中，定位是面向开发者的本地 AI 自动化工作台。

![Version](https://img.shields.io/badge/version-1.0-blue)
![Tests](https://img.shields.io/badge/tests-65%2F65-brightgreen)
![Platform](https://img.shields.io/badge/platform-Windows%2010%2B-blue)
![Qt](https://img.shields.io/badge/Qt-6.x-green)
![License](https://img.shields.io/badge/license-MIT-blue)

## 项目亮点

- 原生 Qt6 Widgets 桌面界面，包含暗色主题、消息编辑、右键菜单、搜索、打字指示器、Token 计数器、Markdown 渲染和代码高亮。
- OpenAI 兼容流式 API 客户端，可对接 DeepSeek、OpenAI 风格接口和自定义服务商。
- Agent 执行循环：分析任务、选择工具、执行动作、观察结果，并持续迭代直到完成或达到最大轮次保护。
- 50+ 本地工具能力，覆盖文件操作、Shell 命令、Git、桌面感知、OCR、输入模拟、HTTP、代码执行、MCP 外部工具和项目搜索。
- 三层记忆系统：用户级记忆、项目级记忆、每日日志。
- Skills 和 Hooks 系统，用于扩展任务指令、生命周期回调和项目级自动化逻辑。
- Cron 风格定时任务调度，并提供可视化管理界面。
- Agent 步骤卡片在聊天流中分组展示，默认折叠，便于查看执行过程。
- Windows 发布流程完整，支持 Release CMake 构建、`windeployqt` 依赖部署和 zip 打包。

## 当前状态

项目处于持续开发状态，当前本地验证基线为：

```powershell
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

预期结果：`65/65` 个测试通过。

## 界面与模式

### 聊天模式

- 支持流式 AI 回复。
- 支持 GitHub 风格 Markdown 渲染和代码高亮。
- 支持消息复制、编辑、删除、重新生成、分支和导出。
- 图片粘贴链路已预留给后续多模态服务商；DeepSeek 个人 API 的图像能力可能受账号/API 状态限制。

### Agent 模式

Agent 可以根据任务自行决定何时调用工具、直接执行工具、追加观察结果，并继续后续推理。命令执行默认面向开发者高权限工作流，同时对 Windows 系统路径的破坏性操作做保护。

工具分组示例：

```text
file.*        read/save/edit/grep/copy/move/append/info/delete/archive/extract/watch
workspace.*   write/read/overwrite/delete/list/create-directory
command.*     bash/git status/git diff/cmake build/ctest
system.*      screen capture/OCR/windows/foreground/control/env/path/clipboard
input.*       mouse/keyboard/text/foreground validation/UI automation
web.*         HTTP GET/POST/download/open URL
code.*        Python/JavaScript snippets
mcp.*         external tools registered through MCP connectors
```

## 架构

CodeXX 采用分层 Qt/C++ 架构：

```text
src/ui        Widgets、聊天视图、对话框、消息渲染、步骤卡片
src/app       ApplicationController、AgentOrchestrator、Prompt 构建器、协调器
src/core      AppConfig、ChatSession、共享领域模型
src/services  OpenAI 兼容 API 客户端、SSE 流解析器、摘要客户端
src/storage   配置、提示词模板、聊天历史、凭据存储设置
src/support   日志和支撑工具
src/tools     本地工具注册表和工具实现
src/memory    项目记忆管理器和每日记忆写入
src/skills    Skills 发现和匹配
src/hooks     Hook 定义和内置 Hooks
src/mcp       MCP 注册表和连接器
src/scheduler Cron 解析器、任务存储、调度器
tests         自定义 assert 测试框架，未使用 QtTest
```

运行时主流程：

```text
MainWindow
  -> ApplicationController
    -> AgentOrchestrator
      -> AgentLoopPromptBuilder
      -> OpenAICompatibleClient
      -> AgentToolRegistry
      -> Memory / Skills / Hooks / MCP / Scheduler
```

## 环境要求

- Windows 10 或 Windows 11。
- Qt 6.x，包含 Core、Widgets、Network、Sql、Gui、Test 模块。
- CMake 3.22+。
- 与已安装 Qt 套件匹配的 MinGW-w64 工具链。
- OpenAI 兼容 API 地址和 API Key。

当前已知本地 Release 环境使用 Qt 6.10.2 和 Qt 自带 MinGW 13.1 工具链。开发构建也可使用本机现有 `build` 目录。

## 构建

开发构建：

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

Release 构建：

```powershell
cmake -S . -B build-release-qt -G "MinGW Makefiles" `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_CXX_COMPILER=D:/QT/Tools/mingw1310_64/bin/g++.exe

cmake --build build-release-qt -j4
ctest --test-dir build-release-qt --output-on-failure
```

如果 CMake 选到了错误的编译器，请删除构建目录，并使用与 Qt 匹配的 MinGW 编译器重新配置。

## 运行

```powershell
.\build\AIChatDesktop.exe
```

首次启动后，在设置窗口中配置：

- Base URL，例如 `https://api.deepseek.com` 或其他 OpenAI 兼容接口。
- API Key。
- 模型名称。
- 可选 temperature/max token 参数。
- Agent 项目目录。

API Key 通过应用本地配置/凭据路径保存，不会包含在发布包中。

## Windows 打包

创建可分发的发布目录和 zip 压缩包：

```powershell
cmake --build build-release-qt -j4
ctest --test-dir build-release-qt --output-on-failure

Remove-Item -Recurse -Force release\AIChatDesktop -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force release\AIChatDesktop
Copy-Item build-release-qt\AIChatDesktop.exe release\AIChatDesktop\

windeployqt release\AIChatDesktop\AIChatDesktop.exe
Compress-Archive -Path release\AIChatDesktop\* `
  -DestinationPath release\AIChatDesktop-1.0-windows.zip `
  -Force
```

如果 `windeployqt` 不在 `PATH` 中，可使用 Qt 安装目录中的完整路径，例如：

```powershell
& "D:\QT\6.10.2\mingw_64\bin\windeployqt.exe" release\AIChatDesktop\AIChatDesktop.exe
```

完整打包检查表见 [docs/05-windows-packaging.md](docs/05-windows-packaging.md)。

## 测试

测试使用轻量级 `assert()` 自定义框架，并通过 CTest 注册：

```powershell
ctest --test-dir build --output-on-failure
```

覆盖范围包括：

- Core 模型和服务商预设。
- App 协调器、Prompt 构建器、Agent 循环解析和执行。
- 流式解析器和 OpenAI 兼容请求体生成。
- 存储和配置服务。
- 文件、命令、Git、OCR、输入、Web、记忆、调度器、Hooks、MCP 和 UI smoke 路径。

## 安全说明

- Agent 命令执行能力较强，适合作为本地开发者自动化工具使用。
- 命令默认在配置的项目目录中执行，同时允许绝对路径。
- 针对受保护 Windows 系统路径的破坏性命令会被阻止。
- 文件和命令输出可能包含本地敏感信息，发布日志或对话记录前应先检查。
- API Key 不应提交到仓库，发布包也不会携带用户凭据。

## 仓库结构

```text
CMakeLists.txt
src/
tests/
docs/
resources/styles/app.qss
```

`build/`、`build-release-qt/`、`release/` 等生成目录不应提交到 Git。

## 许可证

MIT。详见 [LICENSE](LICENSE)。

---

# CodeXX (English)

Qt6/C++ desktop AI coding assistant for Windows. CodeXX combines a streaming chat client, an agentic execution loop, local tools, memory, skills, hooks, MCP integration, and scheduled tasks into a native Widgets application.

![Version](https://img.shields.io/badge/version-1.0-blue)
![Tests](https://img.shields.io/badge/tests-65%2F65-brightgreen)
![Platform](https://img.shields.io/badge/platform-Windows%2010%2B-blue)
![Qt](https://img.shields.io/badge/Qt-6.x-green)
![License](https://img.shields.io/badge/license-MIT-blue)

## Highlights

- Native Qt6 Widgets desktop UI with dark theme, message editing, right-click actions, search, typing indicator, token counter, and Markdown/code rendering.
- OpenAI-compatible streaming API client, tested with DeepSeek/OpenAI-style endpoints.
- Agent loop: analyze, execute tools, observe results, continue until completion or max-iteration guard.
- 50+ local tools for files, shell commands, Git, desktop perception, OCR, input simulation, HTTP, code execution, MCP tools, and project search.
- Three-layer memory model: user memory, project memory, and daily logs.
- Skills and Hooks systems for task-specific instructions and lifecycle callbacks.
- Cron-style scheduled tasks with management UI.
- Agent step cards grouped and collapsible in the chat stream.
- Windows packaging flow based on Release CMake build, `windeployqt`, and zip archive.

## Current Status

The repository is actively developed. The current local validation baseline is:

```powershell
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

Expected result: `65/65` tests pass.

## Screens and Modes

### Chat Mode

- Streaming assistant replies.
- GitHub-flavored Markdown rendering with code highlighting.
- Message copy, edit, delete, regenerate, branch, and export.
- Image paste pipeline is present for future multimodal providers; DeepSeek personal API image support may be unavailable depending on account/API status.

### Agent Mode

The Agent can decide when to call tools, execute them directly, append observations, and continue the loop. Shell execution is intentionally high-permission for developer workflows, while destructive operations targeting Windows system paths are protected.

Example tool groups:

```text
file.*        read/save/edit/grep/copy/move/append/info/delete/archive/extract/watch
workspace.*   write/read/overwrite/delete/list/create-directory
command.*     bash/git status/git diff/cmake build/ctest
system.*      screen capture/OCR/windows/foreground/control/env/path/clipboard
input.*       mouse/keyboard/text/foreground validation/UI automation
web.*         HTTP GET/POST/download/open URL
code.*        Python/JavaScript snippets
mcp.*         external tools registered through MCP connectors
```

## Architecture

CodeXX follows a layered Qt/C++ architecture:

```text
src/ui        Widgets, chat view, dialogs, message rendering, step cards
src/app       ApplicationController, AgentOrchestrator, prompt builders, coordinators
src/core      AppConfig, ChatSession, shared domain models
src/services  OpenAI-compatible API client, SSE stream parser, summary client
src/storage   Config, prompt templates, chat history, credential-backed settings
src/support   Logging and support utilities
src/tools     Local tool registry and tool implementations
src/memory    Project memory manager and daily memory writer
src/skills    Skills discovery and matching
src/hooks     Hook definitions and built-in hooks
src/mcp       MCP registry/connectors
src/scheduler Cron parser, task storage, scheduler
tests         Custom assert-based tests, not QtTest
```

Runtime flow:

```text
MainWindow
  -> ApplicationController
    -> AgentOrchestrator
      -> AgentLoopPromptBuilder
      -> OpenAICompatibleClient
      -> AgentToolRegistry
      -> Memory / Skills / Hooks / MCP / Scheduler
```

## Requirements

- Windows 10 or Windows 11.
- Qt 6.x with Core, Widgets, Network, Sql, Gui, and Test modules.
- CMake 3.22+.
- MinGW-w64 toolchain compatible with the installed Qt package.
- An OpenAI-compatible API endpoint and API key.

The known local release environment uses Qt 6.10.2 with Qt's MinGW 13.1 toolchain. The development build currently also works with the existing `build` directory on this machine.

## Build

Development build:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

Release build:

```powershell
cmake -S . -B build-release-qt -G "MinGW Makefiles" `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_CXX_COMPILER=D:/QT/Tools/mingw1310_64/bin/g++.exe

cmake --build build-release-qt -j4
ctest --test-dir build-release-qt --output-on-failure
```

If CMake finds the wrong compiler, delete the build directory and configure again with the Qt-matching MinGW compiler.

## Run

```powershell
.\build\AIChatDesktop.exe
```

On first launch, open Settings and configure:

- Base URL, for example `https://api.deepseek.com` or another OpenAI-compatible endpoint.
- API Key.
- Model name.
- Optional temperature/max token settings.
- Agent project directory.

API keys are stored through the app's local configuration/credential path and are not included in release packages.

## Package for Windows

Create a deployable release directory and zip archive:

```powershell
cmake --build build-release-qt -j4
ctest --test-dir build-release-qt --output-on-failure

Remove-Item -Recurse -Force release\AIChatDesktop -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force release\AIChatDesktop
Copy-Item build-release-qt\AIChatDesktop.exe release\AIChatDesktop\

windeployqt release\AIChatDesktop\AIChatDesktop.exe
Compress-Archive -Path release\AIChatDesktop\* `
  -DestinationPath release\AIChatDesktop-1.0-windows.zip `
  -Force
```

If `windeployqt` is not on `PATH`, call it from the Qt installation directory, for example:

```powershell
& "D:\QT\6.10.2\mingw_64\bin\windeployqt.exe" release\AIChatDesktop\AIChatDesktop.exe
```

See [docs/05-windows-packaging.md](docs/05-windows-packaging.md) for the full packaging checklist.

## Testing

Tests use a lightweight `assert()`-based framework. They are registered through CTest:

```powershell
ctest --test-dir build --output-on-failure
```

Coverage includes:

- Core models and provider presets.
- App coordinators, prompt builders, Agent loop parsing/execution.
- Stream parser and OpenAI-compatible request body generation.
- Storage and config services.
- File, command, Git, OCR, input, web, memory, scheduler, hooks, MCP, and UI smoke paths.

## Security Notes

- Agent command execution is powerful by design. Treat Agent mode as a local developer automation tool.
- Commands run in the configured project directory by default; absolute paths are allowed.
- Destructive commands targeting protected Windows system paths are blocked.
- File and command outputs may contain sensitive local data. Review logs and shared transcripts before publishing.
- API keys should never be committed. The release package does not contain user credentials.

## Repository Layout

```text
CMakeLists.txt
src/
tests/
docs/
resources/styles/app.qss
```

Generated directories such as `build/`, `build-release-qt/`, and `release/` should not be committed.

## License

MIT. See [LICENSE](LICENSE).
