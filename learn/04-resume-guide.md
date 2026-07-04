# 简历项目表达

本文帮助你把 AI Chat Desktop 写成简历上的项目，并准备一段清晰的口头介绍。

## 1. 项目一句话

AI Chat Desktop 是一个基于 C++17、Qt 6 和 CMake 开发的 Windows 桌面 AI 聊天应用，支持 OpenAI 兼容接口、多会话管理、流式回复、安全凭据存储、错误重试、日志查看、受控本地工具和 Windows 发布打包。

## 2. 简历项目描述示例

可以写成：

```text
AI Chat Desktop | C++17 / Qt 6 / CMake / SQLite / Windows Credential Manager

- 基于 Qt Widgets 开发 Windows 桌面 AI 聊天应用，支持 DeepSeek、OpenAI 等 OpenAI 兼容接口，完成多轮对话、流式回复、停止生成和失败重试。
- 设计分层架构，将界面层、控制层、服务层、存储层和核心模型拆分，使用 ApplicationController 统一管理聊天、会话、配置和网络请求流程。
- 使用 SQLite 保存本地聊天历史，支持多会话新建、切换、重命名、搜索、删除和 Markdown 导出。
- 接入 Windows Credential Manager 保存 API Key，并实现旧 QSettings API Key 迁移，降低敏感信息明文存储风险。
- 设计受控本地工具能力，支持文本工具、用户选择文件读取、目录列出和输出保存；通过文件大小限制、覆盖确认、路径摘要日志降低误操作风险。
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
- 受控本地文件交互
- Git / GitHub Pull Request

工程关键词：

- 分层架构
- 事件驱动
- 异步网络请求
- 本地持久化
- 安全凭据存储
- 自动化测试
- 日志脱敏
- 安全边界设计
- 发布打包
- 迭代开发

## 4. 口头介绍模板

面试时可以这样讲：

```text
这个项目是我用 C++17 和 Qt 6 做的 Windows 桌面 AI 聊天应用。用户可以配置 DeepSeek 或 OpenAI 兼容服务商的 Base URL、模型和 API Key，然后进行多轮对话。

项目上我比较关注完整软件开发流程，不只是把界面做出来。我先写了需求和技术设计，再拆分 V1、V2、V3 任务，逐步实现聊天闭环、多会话、角色提示词、安全存储、错误重试和日志查看。

架构上我把项目分为 ui、app、services、storage、core 几层。UI 层只负责界面和用户交互，ApplicationController 负责业务流程，services 负责网络请求和流式解析，storage 负责 SQLite 聊天记录、QSettings 配置和 Windows Credential Manager 凭据保存。

最后我用 CMake 和 CTest 做构建测试，使用 GitHub feature branch 和 Pull Request 合并流程，并完成 Windows Release 打包和阶段验收文档。
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
- 有权限边界意识，本地文件工具只处理用户选择路径，不做脚本执行或后台自动化。
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

- "接近企业桌面应用开发流程的个人项目"。
- "围绕安全存储、可诊断性、测试和发布做了工程化补齐"。
- "后续可以继续补 UI 自动化测试、全文索引和更完整的 Markdown 渲染"。

---

## 11. v1.0 更新：Agent 平台化表达

v1.0 后，项目已从聊天客户端进化为桌面 Agent 平台，简历应体现这些新能力。

### 更新后的简历项目描述

```text
CodeXX (AI Chat Desktop) 桌面 AI Agent 平台 | C++17 / Qt 6 / Win32 / CMake

- 基于 Qt6 Widgets 开发 Windows 桌面 AI Agent 平台，从基础聊天客户端迭代为完整的 Agent 系统。内置 51 个自动化工具，覆盖文件操作、命令执行、桌面操控、网络请求、代码运行等 7 大领域。
- 实现 Agentic Loop 编排：7 种意图场景自动分类 → ⭐推荐工具优先排序 → 注入最佳实践提示；编辑后自动触发 cmake 构建 + ctest 测试（AutoFix 闭环），重复动作指纹检测防死循环。
- 设计三层记忆系统（L1 用户级 / L2 项目级 / L3 每日日志），自动注入 systemPrompt；15 个 Skill 支持子串匹配与优先级合并；Hook 系统 6 个生命周期钩子。
- 基于 Win32 API + UIAutomation 实现桌面自动化：SendInput 键盘鼠标模拟、UIA 控件定位、窗口枚举、截图 OCR、剪贴板读写。
- 基于 Function Calling 协议实现 51 个工具的统一注册表，7 个 CMake 子库独立编译，69 项自动化测试零回归。
- 使用 CMake/CTest/CI 搭建构建测试流程，GitHub 开源（v1.0, MIT），并启动 Python sidecar 能力层演进。
```

### 更新后的关键词

新增技术关键词：
- **Agentic Loop**、**Function Calling**、**OODA 循环**
- **Win32 API**、**UIAutomation**、**SendInput**、**OCR**
- **三层记忆系统**、**Skill 系统**、**Hook 系统**
- **意图感知工具排序**、**AutoFix 自动修复闭环**
- **Python sidecar**、**JSONL 协议**、**QProcess 子进程能力层**

## 12. STAR 故事 5：Agent 工具系统设计

Situation：项目需要让 AI 不仅聊天，还能操作文件系统、执行命令、操控桌面。

Task：设计一套可扩展的 Agent 工具注册表，让 AI 能安全调用本地能力。

Action：
- 设计 `AgentToolRegistry` 统一注册表，每个工具的 ID、描述、Schema、执行函数集中管理。
- 按领域拆分为 7 个 CMake 子库（registry / core / perception / input / dev / text / assistant）。
- 工具注册时绑定 lambda 闭包，工具描述自动转为 Function Calling schema。
- 对危险操作建立安全边界：系统目录保护、危险命令黑名单、受保护文件校验。

Result：51 个工具，统一注册表，新增工具 10 行代码即可注册。7 个子库独立编译，依赖方向明确。

## 13. STAR 故事 6：桌面自动化闭环

Situation：Agent 只能操作文件，无法像人一样使用 GUI 软件。

Task：实现 Agent 的桌面感知和操作能力。

Action：
- 使用 Win32 `SendInput` 实现键盘（Unicode 输入 + 组合键）和鼠标（点击/拖拽/滚轮/位置查询）。
- 使用 UIAutomation COM 接口实现控件定位（按名称/AutomationId 查找）。
- 实现截图 → OCR 文字提取 → 坐标定位 → 点击输入的完整感知操作链路。
- 加入系统保护窗口黑名单和前台窗口校验，防止误操作。

Result：Agent 可以独立完成"打开浏览器→搜索→截图→OCR→点击→输入"的完整桌面操作。

## 14. STAR 故事 7：Agent 死循环防护

Situation：Agent 循环可能出现无限重复相同操作，消耗 API 额度。

Task：设计多层防护机制。

Action：
- 同步路径：`AgentLoopController` 中维护 `seenActions` 指纹表，同 toolId+同参数 3 次 → 终止。
- 异步路径：`AgentOrchestrator` 中维护 `m_actionFingerprints`，检测后注入警告 observation。
- 上下文管理：Microcompact 压缩早期 observation，Reactive 压缩（API 报 context_length_exceeded 时）。
- Stop Hook：done=true 前验证目标是否完成（提示词注入）。

Result：Agent 循环不会无限运行。重复动作 3 次自动终止，上下文超限自动压缩重试。

## 15. 不建议夸大的点（v1.0 更新）

v1.0 之后，以下表述是**可以说的**：

- "实现了完整的桌面 Agent 系统"
- "具备 51 个自动化工具，覆盖 7 大领域"
- "独立完成从基础聊天应用到 Agent 平台的架构演进"

**仍然不建议说**的：
- "具备企业级高并发能力"（单用户桌面应用）
- "达到商业产品安全标准"（个人项目级别的安全实践）
- "支持全平台"（目前仅 Windows）
