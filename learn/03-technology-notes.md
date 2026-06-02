# 技术栈讲解

本文介绍本项目用到的主要技术，以及它们在代码中的落点。

## 1. C++17

项目使用 C++17 作为主语言。

常见使用点：

- `std::optional`：表示可选模型参数，例如 temperature、max tokens。
- RAII：Qt 对象父子关系、局部对象自动释放、文件对象自动关闭。
- 强类型枚举：例如 `RequestErrorCategory`、`MessageRole`。
- 接口抽象：例如 `LocalTool` 统一本地工具调用方式。

学习重点：

- C++ 项目通常需要明确对象生命周期。
- 尽量让数据模型简单，业务流程放到控制层。
- 不要把界面细节、网络细节和存储细节混在一个类里。

## 2. Qt 6 Widgets

Qt Widgets 用来开发传统桌面 GUI。

项目中的使用：

- `QMainWindow`：主窗口。
- `QDialog`：设置、角色提示词、日志窗口。
- `QPushButton`、`QLineEdit`、`QTextEdit`、`QListWidget`：基础控件。
- `QVBoxLayout`、`QHBoxLayout`：布局管理。
- QSS：样式表，统一控制界面外观。

为什么选择 Qt Widgets：

- C++ 原生桌面开发成熟。
- Windows 桌面应用打包路径清晰。
- 信号槽机制适合事件驱动界面。
- Widgets 比 QML 更适合先学习传统桌面软件架构。

## 3. Qt 信号槽

信号槽是 Qt 的事件通信机制。

项目中的例子：

- 点击发送按钮触发 `sendCurrentMessage()`。
- 控制层发出 `assistantMessageUpdated`，界面更新最后一条 AI 消息。
- 网络客户端发出 `textDeltaReceived`，控制层接收流式文本。

优点：

- 解耦调用方和接收方。
- 适合异步事件，例如网络返回、按钮点击、状态变化。
- 代码可读性比手动回调链更好。

面试表达：

> 本项目没有让 UI 直接处理网络请求，而是通过 Qt 信号槽把用户操作传给控制层，再把业务状态变化通知回 UI。这样界面层和业务层的耦合更低。

## 4. Qt Network

Qt Network 负责 HTTP 请求。

项目中的使用：

- `QNetworkAccessManager` 发起 POST 请求。
- `QNetworkRequest` 设置 URL、Content-Type、Authorization。
- `QNetworkReply` 接收流式数据。
- `readyRead` 信号处理增量返回。
- `finished` 信号处理完成或失败。

关键点：

- Qt Network 是异步模型，不需要手动创建线程。
- 网络事件由 Qt 事件循环驱动。
- 取消请求时需要 abort 当前 `QNetworkReply`。

## 5. OpenAI 兼容接口

DeepSeek 和 OpenAI 都可以使用类似的 chat completions 请求格式。

请求体核心字段：

```json
{
  "model": "model-name",
  "messages": [],
  "stream": true,
  "temperature": 0.7,
  "max_tokens": 2048
}
```

项目中的设计：

- Base URL 和模型名可配置。
- 服务商预设帮用户自动填默认值。
- temperature 和 max tokens 是可选参数。
- 参数为空时不写入请求体，保证兼容。

## 6. SSE 流式响应

SSE 是 Server-Sent Events，服务端可以持续推送数据。

AI 回复流式展示依赖 SSE：

- 用户不用等完整回答生成完。
- 每收到一段文本就更新界面。
- 体验更接近常见 AI 聊天软件。

项目中 `StreamParser` 独立负责解析 SSE，避免网络层和字符串解析混在一起。

## 7. JSON

项目中 JSON 用在两个地方：

- AI 请求体和响应体。
- 角色提示词模板本地保存。
- V5 本地工具中的 JSON 格式化和压缩。

Qt 相关类：

- `QJsonObject`
- `QJsonArray`
- `QJsonDocument`

学习重点：

- 网络请求体不要靠字符串拼接，应该使用 JSON API 构建。
- 解析外部数据时要考虑字段缺失和格式异常。

## 8. SQLite / Qt SQL

SQLite 用来保存聊天历史。

项目中的能力：

- 保存会话。
- 保存消息。
- 加载最近会话。
- 会话搜索。
- 会话收藏。
- 会话归档。
- 会话筛选。
- 删除会话。
- 替换会话消息，支持失败重试后清理旧失败回复。

Qt 相关类：

- `QSqlDatabase`
- `QSqlQuery`
- `QSqlError`

为什么用 SQLite：

- 单文件数据库，适合桌面应用。
- 不需要单独安装数据库服务。
- 支持结构化查询和后续扩展。
- 后续增加收藏、归档这类结构化状态时，比纯 JSON 文件更容易迁移和查询。

## 9. 本地工具抽象

V5 新增了本地工具系统。

核心接口：

- `ToolResult`：统一表示工具执行成功或失败。
- `LocalTool`：统一本地工具 ID、显示名、说明和运行入口。

当前工具：

- JSON 格式化。
- JSON 压缩。
- Markdown 整理。
- 普通文本清理。
- V6 受控文件工具：读取文本文件、列出文件夹、保存输出、打开确认后的路径。

设计价值：

- 工具逻辑不依赖 `MainWindow`。
- 工具可以独立写单元测试。
- 工具输出先给用户确认，不自动发送给 AI。
- 后续如果做受控本地文件/系统交互，可以继续复用这层抽象。

V6 文件交互的关键实现：

- `QFile` 负责文件读写。
- `QFileInfo` 负责判断路径是否存在、是否是文件或目录、文件大小。
- `QDir` 负责列出目录和创建父目录。
- `QFileDialog` 负责让用户显式选择文件或目录。
- `QDesktopServices` 负责把确认后的文件或文件夹交给系统打开。

这部分没有让 AI 自动操作电脑，而是先建立“用户选择路径、程序校验、用户确认、日志记录”的低风险边界。

V7 开始增加受控 Agent 的基础模块：

- `AgentToolCatalog`：定义 AI 可建议的工具 ID、风险等级、输入限制和是否可能包含敏感结果。
- `AgentPlan`：保存 AI 生成的结构化计划和步骤状态。
- `AgentPlanParser`：解析 AI 返回的 JSON 计划，并校验步骤数量、必填字段、工具 ID、参数类型和风险等级。
- `AgentPlanPromptBuilder`：把用户目标和工具目录整理成要求 AI 返回 JSON 计划的提示词。
- `AgentPlanExecutor`：执行用户确认后的低风险文本工具步骤，文件工具仍走专用文件选择流程。
- `AgentPlanDialog`：展示计划、步骤详情、风险、参数和执行输出。

这里的重点是“不信任模型输出”。AI 返回的计划必须先经过本地解析器和工具目录校验，不能直接变成工具执行。

## 10. Windows Credential Manager

API Key 属于敏感信息，不适合写入普通配置文件。

项目中的方案：

- 非敏感配置使用 `QSettings`。
- API Key 使用 Windows Credential Manager。
- 通过 `CredentialStorage` 抽象隔离凭据存储。
- 使用 `WindowsCredentialStorage` 调用 Win32 Credential Management API。

涉及 API：

- `CredWriteW`
- `CredReadW`
- `CredDeleteW`
- `CredFree`

设计价值：

- 降低 API Key 明文泄露风险。
- 业务层不直接依赖 Win32 API。
- 测试中可以用 fake credential storage。

## 11. QSettings

`QSettings` 用于保存普通配置。

项目中保存：

- Base URL。
- 模型名。
- 语言。
- 服务商选择。
- 模型参数。

不再保存：

- API Key。

这是一个重要边界：配置不等于凭据。普通配置可以读写方便，敏感凭据要进入系统级安全存储。

## 12. CMake

CMake 是项目构建系统。

项目中负责：

- 声明 C++ 标准。
- 查找 Qt 6 模块。
- 添加主程序目标。
- 添加资源文件。
- 链接 Qt 和 Windows 库。
- 添加测试子目录。

学习重点：

- C++ 项目通常需要显式管理源文件和依赖库。
- Qt 项目需要 MOC、RCC 等自动生成步骤。
- Windows Credential Manager 需要链接 `Advapi32`。

## 13. CTest

CTest 是 CMake 配套的测试入口。

运行方式：

```powershell
ctest --test-dir build-qt --output-on-failure
```

当前测试覆盖：

- 核心模型。
- 服务商预设。
- 会话列表排序。
- SSE 解析。
- 请求体构建和错误分类。
- 配置和凭据迁移。
- SQLite 存储。
- Markdown 导出。
- 日志脱敏。
- UI smoke test。
- 本地工具逻辑。
- 工具窗口 smoke test。
- 会话收藏、归档和筛选。
- Agent 工具目录和计划解析。
- Agent 计划 Prompt 生成。
- Agent 计划预览和低风险步骤执行。
- Agent 继续规划基础流程。

面试表达：

> 本项目把可测试的逻辑尽量从 UI 中抽出来，例如 SSE 解析、请求体构建、存储、导出、日志读取等都可以通过 CTest 自动化验证。

当前 V7 开发分支测试数量为 27 个。

## 14. windeployqt

`windeployqt` 是 Qt 提供的 Windows 部署工具。

作用：

- 收集 Qt DLL。
- 复制平台插件。
- 复制 SQL 驱动。
- 复制 TLS 插件。
- 复制样式和图片格式插件。

本项目发布时使用：

```powershell
windeployqt release\AIChatDesktop\AIChatDesktop.exe
```

注意点：

- 构建用的 Qt 和运行依赖必须匹配。
- 如果缺少 `qwindows.dll`，应用无法启动。
- 如果缺少 SQLite 驱动，聊天记录可能无法打开。
- HTTPS 需要 TLS 插件和系统证书支持。

## 15. Git / GitHub 协作流程

本项目按 feature branch 方式迭代：

1. 从 `main` 创建 `codex/...` 分支。
2. 在分支上完成一个功能或修复。
3. 本地构建和测试。
4. 提交 commit。
5. 推送到 GitHub。
6. 创建 Pull Request。
7. 合并回 `main`。
8. 本地 pull 同步。

这套流程适合写进简历：

> 实践 Git/GitHub feature branch、Pull Request、代码提交、阶段验收和发布说明同步流程，保持 main 分支稳定。
