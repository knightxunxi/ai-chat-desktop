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
- 应用启动/关闭 smoke test。

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

发布前请在 `release\AIChatDesktop\AIChatDesktop.exe` 中手工验证 API 请求、语言切换、聊天记录恢复、停止生成、消息复制、Markdown 展示、角色提示词模板、错误重试和应用内日志查看。

## 当前版本状态

当前 main 已完成 V3，覆盖安全存储、会话管理增强、服务商预设、模型参数、错误重试、应用内日志查看和发布准备文档。

已知限制：

- 当前 V3 只面向 Windows，暂未规划 macOS/Linux 迁移。
- Markdown 展示仍是基础版本，复杂代码高亮暂未支持。
- 角色提示词模板仍是本地基础版本，尚未支持导入导出或云同步。

## 后续方向

V3 验收和复盘见 [V3 验收与复盘](docs/11-v3-acceptance-notes.md)。

V3 详细计划见 [V3 Roadmap](docs/09-v3-roadmap.md)，发布说明见 [V3 Release Notes](docs/12-v3-release-notes.md)。

候选方向：

- GitHub Actions CI 和基础工程规范。
- Markdown 代码高亮增强。
- 角色提示词模板导入导出。
