# CodeXX

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
