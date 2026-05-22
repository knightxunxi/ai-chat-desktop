# AI Chat Desktop

一个基于 C++、Qt 6 和 CMake 的桌面 AI 聊天应用。用户可以配置 DeepSeek 或其他 OpenAI 兼容接口的 API Key、Base URL 和模型名称，然后进行多轮对话。

本项目同时作为软件开发流程学习项目，包含需求说明、技术方案、任务拆分、验收记录、Windows 打包说明和阶段复盘。

## 功能特性

- OpenAI 兼容聊天接口。
- DeepSeek 默认配置。
- API Key、Base URL、模型名称配置。
- 中文/英文界面语言切换。
- 当前会话多轮上下文。
- 当前会话角色提示词。
- AI 回复流式展示。
- 本地保存配置。
- SQLite 保存聊天记录。
- 多会话列表、新建、切换和删除。
- 停止生成。
- 消息复制。
- AI 消息基础 Markdown 渲染。
- 角色提示词模板。
- 基础错误提示。
- 基础日志记录。
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

当前测试覆盖：

- 核心模型。
- SSE 流式响应解析。
- OpenAI 兼容请求体构建。
- SQLite 聊天记录存储。
- 设置窗口基础行为。
- 消息复制和 Markdown 展示行为。
- 样式资源加载。
- 基础日志写入和敏感字段脱敏。
- 角色提示词模板保存和选择。

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

发布前请在 `release\AIChatDesktop\AIChatDesktop.exe` 中手工验证 API 请求、语言切换、聊天记录恢复、停止生成、消息复制、Markdown 展示和角色提示词模板。

## 当前版本状态

V2 已完成基础聊天闭环、多会话、停止生成、消息复制、基础 Markdown 展示、基础日志和角色提示词模板。

已知限制：

- API Key 使用普通本地配置保存，尚未加密。
- 多会话管理缺少重命名和搜索。
- Markdown 展示仍是基础版本，复杂代码高亮暂未支持。
- 日志只覆盖基础请求生命周期，尚未提供界面内日志查看。
- 角色提示词模板仍是本地基础版本，尚未支持导入导出或云同步。

## 后续方向

V2 验收和复盘见 [V2 验收与复盘](docs/08-v2-acceptance-notes.md)。

V3 候选方向：

- API Key 安全存储。
- 会话重命名、搜索和导出。
- 多服务商预设和模型参数配置。
- 更清晰的错误分类和重试入口。
- 应用内日志查看。
