# 简历项目表达

本文帮助你把 AI Chat Desktop 写成简历上的项目，并准备一段清晰的口头介绍。

## 1. 项目一句话

AI Chat Desktop 是一个基于 C++17、Qt 6 和 CMake 开发的 Windows 桌面 AI 聊天应用，支持 OpenAI 兼容接口、多会话管理、流式回复、安全凭据存储、错误重试、日志查看和 Windows 发布打包。

## 2. 简历项目描述示例

可以写成：

```text
AI Chat Desktop | C++17 / Qt 6 / CMake / SQLite / Windows Credential Manager

- 基于 Qt Widgets 开发 Windows 桌面 AI 聊天应用，支持 DeepSeek、OpenAI 等 OpenAI 兼容接口，完成多轮对话、流式回复、停止生成和失败重试。
- 设计分层架构，将界面层、控制层、服务层、存储层和核心模型拆分，使用 ApplicationController 统一管理聊天、会话、配置和网络请求流程。
- 使用 SQLite 保存本地聊天历史，支持多会话新建、切换、重命名、搜索、删除和 Markdown 导出。
- 接入 Windows Credential Manager 保存 API Key，并实现旧 QSettings API Key 迁移，降低敏感信息明文存储风险。
- 使用 CMake/CTest 搭建构建和测试流程，覆盖 SSE 流解析、请求体构建、HTTP 错误分类、SQLite 存储、配置迁移、日志脱敏等核心逻辑。
- 实践 Git/GitHub feature branch 与 Pull Request 流程，完成需求分析、技术设计、任务拆分、阶段验收、发布说明和 Windows 打包。
```

## 3. 简历上可以突出的关键词

技术关键词：

- C++17
- Qt 6 Widgets
- Qt Network
- Qt SQL / SQLite
- CMake / CTest
- Windows Credential Manager
- OpenAI-compatible API
- Server-Sent Events
- Git / GitHub Pull Request

工程关键词：

- 分层架构
- 事件驱动
- 异步网络请求
- 本地持久化
- 安全凭据存储
- 自动化测试
- 日志脱敏
- 发布打包
- 迭代开发

## 4. 口头介绍模板

面试时可以这样讲：

```text
这个项目是我用 C++17 和 Qt 6 做的 Windows 桌面 AI 聊天应用。用户可以配置 DeepSeek 或 OpenAI 兼容服务商的 Base URL、模型和 API Key，然后进行多轮对话。

项目上我比较关注完整软件开发流程，不只是把界面做出来。我先写了需求和技术设计，再拆分 V1、V2、V3 任务，逐步实现聊天闭环、多会话、角色提示词、安全存储、错误重试和日志查看。

架构上我把项目分为 ui、app、services、storage、core 几层。UI 层只负责界面和用户交互，ApplicationController 负责业务流程，services 负责网络请求和流式解析，storage 负责 SQLite 聊天记录、QSettings 配置和 Windows Credential Manager 凭据保存。

最后我用 CMake 和 CTest 做构建测试，使用 GitHub feature branch 和 Pull Request 合并流程，并完成 Windows Release 打包和 V3 验收文档。
```

## 5. STAR 故事 1：API Key 安全存储

Situation：早期版本把 API Key 当作普通配置保存，存在明文泄露风险。

Task：在只面向 Windows 的前提下，设计更安全的本地凭据存储方案，同时兼容旧配置。

Action：

- 调研 Windows Credential Manager、Qt Keychain 和 QSettings。
- 新增 `CredentialStorage` 抽象，避免业务代码直接依赖 Win32 API。
- 实现 `WindowsCredentialStorage`，使用 `CredWriteW`、`CredReadW` 和 `CredDeleteW`。
- 修改 `ConfigStorage`，非敏感配置仍用 QSettings，API Key 单独保存到系统凭据。
- 加入旧 `api/apiKey` 迁移逻辑和自动化测试。

Result：新保存的 API Key 不再进入普通配置文件，旧版本配置也可以迁移，测试覆盖保存、读取、删除和迁移路径。

## 6. STAR 故事 2：流式回复

Situation：AI 聊天如果等完整回复结束再显示，用户体验比较差。

Task：实现类似常见 AI 聊天软件的流式输出。

Action：

- 使用 Qt Network 发起异步 HTTP 请求。
- 通过 `readyRead` 接收服务端增量数据。
- 单独实现 `StreamParser` 解析 SSE 数据。
- 控制层接收文本增量并更新最后一条 AI 消息。
- 为 SSE 解析增加测试，覆盖增量内容和完成标记。

Result：AI 回复可以边生成边展示，用户也可以中途停止生成。

## 7. STAR 故事 3：会话顺序 bug

Situation：用户点击已有会话时，会话列表顺序会变化，体验不稳定。

Task：区分“查看会话”和“更新会话”，让普通切换不改变列表顺序。

Action：

- 分析会话保存和摘要列表更新逻辑。
- 引入 `moveToTop` 参数，只有真正更新内容时才置顶。
- 抽出 `SessionSummaryList` 维护列表更新规则。
- 增加测试覆盖切换、更新和排序行为。

Result：点击会话只切换当前内容，不会打乱列表；发送新消息时仍能按更新时间排序。

## 8. STAR 故事 4：错误分类和重试

Situation：早期请求失败只显示原始错误，用户不容易判断是 API Key、额度、模型名还是网络问题。

Task：提供更清楚的错误提示，并允许用户重试上一条消息。

Action：

- 新增 `RequestErrorCategory`。
- 根据 HTTP 状态码和网络错误分类。
- 控制层生成中英文友好提示。
- 请求失败后保留上一条用户消息，界面显示“重试”按钮。
- 重试时删除失败回复，避免旧失败内容进入上下文。
- 修正存储层，使用整体替换方式避免数据库中残留失败消息。

Result：用户可以更快定位配置或网络问题，并直接重试。

## 9. 面试中可强调的工程能力

可以强调：

- 不是只做 demo，而是按需求、设计、任务、实现、测试、验收、发布推进。
- 有真实迭代过程，修过界面、存储、排序、关闭占用等 bug。
- 有安全意识，API Key 不写普通配置，日志做脱敏。
- 有测试意识，核心逻辑通过 CTest 覆盖。
- 有发布意识，能生成 Windows Release 包。
- 有协作意识，使用 GitHub branch 和 Pull Request 流程。

## 10. 不建议夸大的点

不要说：

- “实现了完整企业级系统”。
- “具备高并发能力”。
- “实现了复杂 AI Agent 框架”。
- “安全性已经达到商业产品标准”。

更稳妥的说法：

- “接近企业桌面应用开发流程的个人项目”。
- “围绕安全存储、可诊断性、测试和发布做了工程化补齐”。
- “后续可以继续补 UI 自动化测试、全文索引和更完整的 Markdown 渲染”。
