# AI 聊天桌面应用技术方案

## 1. 文档目的

本文档用于说明 AI 聊天桌面应用 V1 的技术选型、架构设计、模块划分、核心流程和关键实现策略。

它对应需求文档中的 V1 范围，目标是让后续开发有明确依据，避免一边写代码一边临时决定架构。

## 2. 技术选型

### 2.1 开发语言

使用 C++。

原因：

- 符合项目学习目标。
- 适合桌面应用开发。
- 可以训练工程化的项目组织、内存管理和模块设计能力。

### 2.2 GUI 框架

使用 Qt 6 Widgets。

原因：

- Qt 对 C++ 桌面开发支持成熟。
- Qt Widgets 适合传统桌面应用和工具型软件。
- Qt 自带信号槽机制，适合处理 UI 与异步网络请求之间的通信。
- Qt 提供网络、JSON、本地配置、数据库等常用能力，减少额外依赖。

V1 不选择 Qt Quick/QML，主要是为了降低学习和工程复杂度。后续如果希望做更现代的动画和界面，可以再评估 Qt Quick。

### 2.3 构建系统

使用 CMake。

原因：

- CMake 是 C++ 项目中常见的跨平台构建系统。
- Qt 6 对 CMake 支持良好。
- 便于后续接入测试、打包和持续集成。

### 2.4 网络请求

使用 Qt Network 模块。

核心类：

- `QNetworkAccessManager`
- `QNetworkRequest`
- `QNetworkReply`

原因：

- 与 Qt 事件循环天然集成。
- 不需要额外引入 libcurl 等第三方库。
- 支持异步请求，不阻塞 UI 线程。

### 2.5 JSON 处理

使用 Qt Core 提供的 JSON 能力。

核心类：

- `QJsonDocument`
- `QJsonObject`
- `QJsonArray`
- `QJsonValue`

### 2.6 本地配置

V1 使用 `QSettings` 保存配置。

保存内容：

- Base URL
- 模型名称
- API Key
- 最近使用的会话 ID

角色提示词不放在 `AppConfig` 中。它属于会话级别数据，应保存在 `ChatSession` 和聊天记录存储中。

注意：

- V1 中 API Key 先以普通本地配置方式保存。
- 这满足学习和最小可用目标，但不属于高安全存储方案。
- 后续版本可以考虑 Windows Credential Manager 或加密存储。

### 2.7 聊天记录存储

V1 使用 SQLite 保存聊天记录。

原因：

- 比 JSON 文件更接近企业项目中的结构化存储方式。
- 方便按会话、时间、角色查询消息。
- Qt 通过 Qt SQL 模块支持 SQLite。

核心类：

- `QSqlDatabase`
- `QSqlQuery`
- `QSqlError`

### 2.8 样式方案

使用 Qt Style Sheet，也就是 `.qss` 文件。

原因：

- 能较快实现接近常见 AI 聊天软件的视觉风格。
- 样式与 C++ UI 逻辑分离。
- 后续可以逐步替换和优化。

## 3. 外部 API 方案

### 3.1 API 类型

V1 使用 OpenAI 兼容的 Chat Completions API。

DeepSeek 官方文档当前说明其 API 兼容 OpenAI 格式，推荐 OpenAI 兼容 Base URL 为：

```text
https://api.deepseek.com
```

V1 默认模型使用：

```text
deepseek-v4-flash
```

说明：`deepseek-chat` 是旧模型名，当前仍可兼容使用，但 DeepSeek 官方文档说明该名称将于 2026-07-24 弃用。

参考资料：

- [DeepSeek API Docs](https://api-docs.deepseek.com/)

### 3.2 请求地址

应用内部将用户配置的 Base URL 与接口路径组合。

默认组合：

```text
POST https://api.deepseek.com/chat/completions
```

为兼容部分服务商，如果用户填写的 Base URL 已经包含 `/v1`，应用不额外强制修改。

### 3.3 请求头

```http
Content-Type: application/json
Authorization: Bearer <API_KEY>
```

### 3.4 请求体

V1 请求体示例：

```json
{
  "model": "deepseek-v4-flash",
  "messages": [
    {
      "role": "system",
      "content": "你是一名严谨但耐心的 C++ 导师。"
    },
    {
      "role": "user",
      "content": "你好"
    }
  ],
  "stream": true
}
```

### 3.5 响应模式

V1 默认使用流式响应。

流式接口通常以 Server-Sent Events 形式返回数据，常见片段形态类似：

```text
data: {"choices":[{"delta":{"content":"你好"}}]}

data: [DONE]
```

应用需要逐段解析 `data:` 后面的 JSON，并将新增文本追加到当前 AI 消息。

## 4. 总体架构

项目采用分层架构。

```text
UI 层
  负责窗口、控件、用户交互和界面状态展示

Core 层
  负责业务模型、会话状态、消息结构和流程控制

Service 层
  负责外部 AI API 调用、流式响应解析和错误转换

Storage 层
  负责配置、聊天记录、本地数据库读写

Resources 层
  负责样式、图标等静态资源
```

设计原则：

- UI 层不直接拼接 HTTP 请求。
- Service 层不直接操作 UI 控件。
- Storage 层不关心界面展示方式。
- Core 层中的数据结构尽量保持简单、稳定。
- 层与层之间通过明确接口和 Qt 信号槽通信。

## 5. 推荐目录结构

```text
ai-chat-desktop/
├─ CMakeLists.txt
├─ docs/
│  ├─ 01-requirements.md
│  └─ 02-technical-design.md
├─ src/
│  ├─ main.cpp
│  ├─ app/
│  │  ├─ ApplicationController.h
│  │  └─ ApplicationController.cpp
│  ├─ core/
│  │  ├─ AppConfig.h
│  │  ├─ ChatMessage.h
│  │  ├─ ChatSession.h
│  │  └─ MessageRole.h
│  ├─ services/
│  │  ├─ AIClient.h
│  │  ├─ OpenAICompatibleClient.h
│  │  ├─ OpenAICompatibleClient.cpp
│  │  ├─ StreamParser.h
│  │  └─ StreamParser.cpp
│  ├─ storage/
│  │  ├─ ConfigStorage.h
│  │  ├─ ConfigStorage.cpp
│  │  ├─ ChatHistoryStorage.h
│  │  └─ ChatHistoryStorage.cpp
│  └─ ui/
│     ├─ MainWindow.h
│     ├─ MainWindow.cpp
│     ├─ ChatView.h
│     ├─ ChatView.cpp
│     ├─ MessageWidget.h
│     ├─ MessageWidget.cpp
│     ├─ SettingsDialog.h
│     └─ SettingsDialog.cpp
├─ resources/
│  ├─ resources.qrc
│  └─ styles/
│     └─ app.qss
└─ tests/
   └─ CMakeLists.txt
```

## 6. 核心模块设计

### 6.1 Core 模块

#### `AppConfig`

负责表示应用配置。

主要字段：

- `providerName`
- `baseUrl`
- `modelName`
- `apiKey`

#### `ChatMessage`

负责表示一条聊天消息。

主要字段：

- `id`
- `sessionId`
- `role`
- `content`
- `createdAt`

#### `ChatSession`

负责表示一个聊天会话。

主要字段：

- `id`
- `title`
- `systemPrompt`
- `createdAt`
- `updatedAt`
- `messages`

`systemPrompt` 用于保存当前会话的角色提示词。它不一定作为普通消息显示在聊天窗口中，但在请求 AI 时会转换为第一条 `system` 消息。

### 6.2 UI 模块

#### `MainWindow`

主窗口，负责整体布局。

主要职责：

- 显示左侧会话区域。
- 显示聊天内容区域。
- 显示输入区域。
- 显示和编辑当前会话的角色提示词入口。
- 打开设置窗口。
- 将用户操作转发给应用控制层。

#### `ChatView`

聊天消息展示区域。

主要职责：

- 添加用户消息。
- 添加 AI 消息。
- 更新正在流式输出的 AI 消息。
- 自动滚动到底部。

#### `SettingsDialog`

设置窗口。

主要职责：

- 展示当前配置。
- 校验用户输入。
- 保存配置。

### 6.3 App 模块

#### `ApplicationController`

应用流程控制器，用于连接 UI、Service 和 Storage。

主要职责：

- 加载配置和历史记录。
- 处理发送消息流程。
- 调用 AI 客户端。
- 接收流式响应并更新会话。
- 触发聊天记录保存。
- 处理错误并通知 UI。

这个类可以避免把业务流程全部写进 `MainWindow`。

### 6.4 Service 模块

#### `AIClient`

抽象接口，定义 AI 客户端能力。

主要能力：

- 发送聊天请求。
- 返回流式文本片段。
- 通知请求完成。
- 通知请求失败。
- 取消当前请求。

#### `OpenAICompatibleClient`

OpenAI 兼容接口实现。

主要职责：

- 根据 `AppConfig` 组装请求。
- 将 `ChatMessage` 转成 API 消息格式。
- 发起 HTTP 请求。
- 读取流式响应。
- 将错误响应转成应用可展示错误。

#### `StreamParser`

流式响应解析器。

主要职责：

- 接收原始响应字节。
- 按行解析 SSE 数据。
- 识别 `[DONE]`。
- 提取 `delta.content`。
- 处理半包数据。

### 6.5 Storage 模块

#### `ConfigStorage`

配置读写。

主要职责：

- 保存 `AppConfig`。
- 加载 `AppConfig`。
- 判断配置是否完整。

#### `ChatHistoryStorage`

聊天记录读写。

主要职责：

- 初始化 SQLite 数据表。
- 保存会话。
- 保存消息。
- 加载最近会话。
- 清空当前会话。

## 7. 数据库设计

V1 推荐两张表。

### 7.1 sessions

```sql
CREATE TABLE sessions (
    id TEXT PRIMARY KEY,
    title TEXT NOT NULL,
    system_prompt TEXT NOT NULL DEFAULT '',
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);
```

### 7.2 messages

```sql
CREATE TABLE messages (
    id TEXT PRIMARY KEY,
    session_id TEXT NOT NULL,
    role TEXT NOT NULL,
    content TEXT NOT NULL,
    created_at TEXT NOT NULL,
    FOREIGN KEY(session_id) REFERENCES sessions(id)
);
```

V1 先保留单会话体验，但数据库按多会话结构设计，方便 V2 扩展。

## 8. 核心流程

### 8.1 应用启动流程

```text
main.cpp
  -> 创建 QApplication
  -> 加载样式
  -> 创建 Storage
  -> 创建 Service
  -> 创建 ApplicationController
  -> 创建 MainWindow
  -> 加载配置和历史记录
  -> 显示主窗口
```

### 8.2 保存配置流程

```text
用户打开设置窗口
  -> SettingsDialog 显示当前配置
  -> 用户修改并点击保存
  -> SettingsDialog 校验输入
  -> MainWindow 通知 ApplicationController
  -> ConfigStorage 保存配置
  -> UI 显示保存成功
```

### 8.3 发送消息流程

```text
用户输入消息并点击发送
  -> MainWindow 获取输入内容
  -> ApplicationController 校验配置和消息
  -> 读取当前会话角色提示词
  -> 创建用户消息并显示
  -> 创建空 AI 消息并显示生成中状态
  -> OpenAICompatibleClient 将角色提示词作为 system 消息放在请求开头
  -> OpenAICompatibleClient 发起流式请求
  -> StreamParser 解析文本片段
  -> ApplicationController 追加文本到 AI 消息
  -> ChatView 更新显示
  -> 请求完成后保存消息
```

### 8.4 错误处理流程

```text
网络或 API 请求失败
  -> OpenAICompatibleClient 识别错误
  -> 转换为用户可读错误
  -> ApplicationController 通知 UI
  -> UI 停止生成状态
  -> 当前失败消息标记为错误或显示错误提示
```

## 9. 错误处理策略

### 9.1 配置错误

场景：

- API Key 为空。
- Base URL 为空。
- 模型名称为空。

处理：

- 阻止发送请求。
- 提示用户打开设置窗口补充配置。

### 9.2 网络错误

场景：

- 无法连接服务器。
- DNS 解析失败。
- 请求超时。
- TLS 错误。

处理：

- 显示简洁错误信息。
- 保留用户输入内容。
- 不清空当前会话。

### 9.3 API 错误

场景：

- API Key 无效。
- 模型名称错误。
- 余额不足。
- 请求参数错误。

处理：

- 优先解析服务端返回的错误信息。
- 如果无法解析，显示 HTTP 状态码和通用错误。

### 9.4 流式解析错误

场景：

- 返回内容不是合法 JSON。
- 流式内容异常中断。

处理：

- 保留已经收到的文本。
- 显示生成中断提示。
- 记录调试日志。

## 10. UI 设计原则

V1 界面参考常见 AI 聊天软件，但不做复杂动画。

主要设计要求：

- 左侧为会话区域。
- 右侧为聊天区域。
- 底部为输入区域。
- 当前会话应提供角色提示词入口，可以用会话设置、顶部按钮或轻量编辑区实现。
- 用户消息靠右或使用明显区分样式。
- AI 消息靠左或使用明显区分样式。
- 输入区域在请求中可以保持可见。
- 发送按钮在空输入时禁用。
- 请求中显示生成状态。

## 11. 测试策略

V1 至少包含以下验证。

### 11.1 手工测试

- 首次启动应用。
- 保存配置。
- 重启后读取配置。
- 发送一条消息。
- 设置角色提示词后发送消息。
- 连续发送多条消息。
- 清空会话。
- 输入空消息。
- 使用错误 API Key。
- 使用错误模型名称。
- 断网后发送消息。

### 11.2 自动化测试

优先为非 UI 逻辑添加测试。

推荐测试对象：

- `StreamParser`
- `ConfigStorage`
- `ChatHistoryStorage`
- API 请求体构建逻辑

## 12. 打包方案

V1 先使用 Qt 提供的 Windows 部署工具处理依赖。

推荐步骤：

```text
1. 使用 CMake 构建 Release 版本。
2. 使用 windeployqt 收集 Qt 运行时依赖。
3. 将可执行文件、依赖库、资源文件放入发布目录。
4. 手工运行发布目录中的应用进行验证。
```

后续可以考虑：

- NSIS 安装包。
- Inno Setup 安装包。
- GitHub Release 发布。

## 13. 开发规范建议

### 13.1 Git 分支

学习阶段可以先使用简单分支模型。

- `main`：稳定分支。
- `feature/*`：功能开发分支。
- `docs/*`：文档更新分支。

### 13.2 提交信息

建议使用简单 Conventional Commits 风格。

示例：

```text
docs: add technical design
feat: add settings dialog
feat: implement openai compatible client
fix: handle empty api key before request
test: add stream parser tests
```

### 13.3 代码风格

- 类名使用 PascalCase。
- 函数名使用 camelCase。
- 成员变量可使用 `m_` 前缀。
- 头文件使用 include guard 或 `#pragma once`。
- UI 文本集中管理，便于后续国际化。

## 14. 后续待确认事项

以下事项可以在进入实现前进一步确认：

- 是否从 V1 就实现完整多会话 UI。
- 是否使用系统默认代理访问 API。
- 聊天记录数据库存储路径。
- API Key 是否在 V1 就尝试加密存储。
- 是否需要为应用设置中文名称和图标。
