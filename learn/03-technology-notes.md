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

## 16. Windows API 与桌面自动化（V14+ / V18.2+）

Agent 模式的核心能力来源：让程序像人一样操作 Windows GUI。

### Win32 键盘模拟 — SendInput

```cpp
INPUT in = {};
in.type = INPUT_KEYBOARD;
in.ki.wScan = ch.unicode();
in.ki.dwFlags = KEYEVENTF_UNICODE;  // 支持 Unicode 字符
SendInput(1, &in, sizeof(INPUT));
in.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;  // 释放键
SendInput(1, &in, sizeof(INPUT));
```

v1.0 支持：文本输入（中英文）、组合键（Ctrl+C/Alt+F4）、单键（Enter/Tab/Esc/方向键）、鼠标点击拖拽滚轮。

### Win32 鼠标模拟

```cpp
SetCursorPos(x, y);  // 移动到屏幕坐标
INPUT down = {}; down.type = INPUT_MOUSE; down.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
INPUT up = {}; up.type = INPUT_MOUSE; up.mi.dwFlags = MOUSEEVENTF_LEFTUP;
SendInput(1, &down, sizeof(INPUT));  // 按下
SendInput(1, &up, sizeof(INPUT));    // 释放
```

### Windows UI Automation (UIA)

通过 COM 接口定位和操作 UI 控件：

```cpp
IUIAutomation *uia = nullptr;
CoCreateInstance(__uuidof(CUIAutomation), nullptr, CLSCTX_INPROC_SERVER,
    __uuidof(IUIAutomation), (void**)&uia);
IUIAutomationElement *el = nullptr;
uia->ElementFromHandle(hwnd, &el);
el->get_CurrentName(&name);      // 控件名
el->get_CurrentBoundingRectangle(&rc); // 控件位置
```

用于：click_button（按名称定位按钮）、active_control（获取焦点控件信息）。

### 窗口管理

- `EnumWindows` — 枚举所有可见窗口。
- `GetForegroundWindow` — 获取前台窗口句柄。
- `GetWindowRect` — 获取窗口位置和大小。
- `GetWindowTextW` — 获取窗口标题。
- 系统窗口黑名单保护（Task Manager / UAC / Ctrl+Alt+Del 屏幕）。

## 17. Agent 工具注册表设计（V8.3+ / V15.4+）

v1.0 的 51 个工具统一注册在 `AgentToolRegistry`。

每个工具的元数据：工具 ID、中英文名称描述、风险等级、JSON Schema、执行函数（lambda 闭包）。

```cpp
definitions.append(makeDefinition(
    makeDescriptor("file.edit_text", "Edit", "编辑",
        "Replace old_str with new_str in file", "精确替换文本",
        "...", AgentToolRisk::Medium, false),
    paramsSchema, allowsDirectExecution,
    [](const QJsonObject &params, const AgentToolExecutionContext &ctx) {
        return FileInteractionService::editTextFile(...);
    }));
```

设计价值：Prompt 和执行器共享同一个注册表，不存在两份工具定义。Function Calling schema 从同一份定义生成。新增工具只需在注册表中追加一个条目。7 个 CMake 子库独立编译。

## 18. 三层记忆系统设计（V13+）

```text
L1 (~/.codex/MEMORY.md)           用户级跨项目偏好     手动写入
L2 (.workbuddy/memory/MEMORY.md)  项目级技术决策       手动写入
L3 (YYYY-MM-DD.md)                每日工作日志         Agent 自动追加
```

关键设计：敏感内容自动拒绝（正则匹配 api_key/token/password 等）。L3 14 天窗口 + 50 条上限 + 30K 字符硬上限。超期日志 ISO 周压缩（LLM 摘要 → compressed.md）。

## 19. Skill 系统设计（V13.3+）

Skill 是场景化工作流指令。YAML frontmatter + Markdown 体，双目录扫描，子串匹配触发词，优先级合并。当前 15 个 Skill（3 工作流 + 12 工具）。

## 20. Agent 循环提示词设计（V18.6）

v1.0 提示词结构：意图标签 + 工具使用提示 + 匹配 Skill + ⭐ 排序工具目录。匹配工具优先加 ⭐，非匹配软限制 30 个。完成后自动记录工具序列。

## 21. v1.0 测试统计

当前全量 68 个测试，100% 通过。覆盖核心模型、服务、存储、工具、Agent、记忆、Skills、Hooks、UI、输入、桌面和 Python sidecar 等层级。CI 双重构建（Debug/Release），GitHub Actions 自动触发。

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

---

## 设计决策与取舍

> 这部分回答"为什么这么做而不是那样做"——面试时被追问架构决策的核心弹药。

### 决策 1：为什么 ApplicationController 拆成 3 个 Coordinator，而不是全删掉？

```
替代方案：每个 Coordinator 各自独立，不需要 ApplicationController
实际选择：保留 ApplicationController 作为门面，3 个 Coordinator 作为成员
```

**为什么**：ApplicationController 持有 `m_aiClient`（AI 客户端实例），Chat 消息流（sendMessage→handleTextDelta→handleRequestFinished）需要它。如果删掉 AC，Chat 模式和 Agent 模式会变成两个独立的控制器，信号槽连接会加倍复杂。保留 AC 做门面，Chat 路径直走 AC，Agent 路径通过 AgentOrchestrator——两种模式的代码不需要互相了解对方的存在。

**面试表达**：这是 Facade 模式的应用——对外提供简单接口，内部委托给子系统。

### 决策 2：为什么不直接用 QTest/GoogleTest 而是手写测试框架？

```
替代方案：QTest 框架（Qt 官方）、Google Test（业界标准）
实际选择：自定义轻量级 assert + CTest
```

**为什么**：QTest 需要 Qt 事件循环，Google Test 需要额外编译依赖。手写框架的测试入口是 `int main() { assert(...); }`——不需要链接任何框架库。当前 68 个测试大多是纯逻辑测试，不依赖窗口和事件循环。另外，手写测试意味着你对每个断言的发生位置和失败信息有完全控制。

**面试表达**：在不需要 mock/spy 的纯逻辑测试场景下，轻量方案往往比重框架更实用。

### 决策 3：为什么 Skill 用 YAML frontmatter 而不是单独的 YAML/JSON 文件？

```
替代方案：skill.yaml + skill.md 双文件；纯 JSON 配置文件
实际选择：SKILL.md 单文件，YAML frontmatter 嵌入在 Markdown 头部
```

**为什么**：(1) 单文件分发零碎依赖——复制一个 SKILL.md 就完成部署；(2) Markdown 是 AI 的原生输入格式——YAML 部分给解析器看，Markdown 体直接注入 prompt，不需要格式转换；(3) 手写 frontmatter 解析器 120 行代码搞定，省去引入 YAML 库的编译依赖。代价是语法容错性不如正式 YAML 库，但对 SKILL.md 这种人工编写的小文件完全够用。

### 决策 4：为什么 HTTP 请求用 QEventLoop 同步等待，而不是 Qt 信号槽异步？

```
替代方案：全程信号槽异步（readyRead → textDeltaReceived → 下一个请求）
实际选择：Agent 循环中 HTTP 请求用 QEventLoop + 超时定时器同步等待
```

**为什么**：Agent 循环是一个 OODA 循环——每轮必须等工具执行完才能进入下一轮。如果用信号槽异步，循环体会分裂成多个回调，代码变成回调地狱。QEventLoop::exec() 阻塞当前协程等待结果，让 Agent 循环的逻辑保持线性可读。代价是阻塞时 Qt 事件循环仍然运转（这是 QEventLoop 的设计），所以 UI 不会卡死。

**面试表达**：这是"局部同步，全局异步"——Agent 循环内部等待单步结果，但 Qt 主事件循环仍然处理 UI 事件。

### 决策 5：为什么工具不直接用 Function Calling 返回的参数，而要再走 AgentToolRegistry 校验？

```
替代方案：模型返回 tool_calls → 直接调用 QProcess/SendInput
实际选择：tool_calls → AgentToolRegistry.find(functionName) → 校验参数 → lambda 执行
```

**为什么**：函数名必须映射到已注册工具——防止模型幻觉出不存在的工具。参数必须经过 JSON Schema 检查。执行函数是注册时绑定的 lambda 闭包，模型无法绕过——比如 command.bash 的危险命令黑名单在 lambda 内部检查，模型返回的任何参数都要经过它。

**面试表达**：这是"模型负责建议，本地负责权限"的 Agent 安全原则。Function Calling 只是信道，不是信任。

### 决策 6：为什么记忆用文件而不是 SQLite？

```
替代方案：SQLite 存记忆（结构化查询 + FTS 搜索）
实际选择：纯 Markdown 文件（~/.codex/MEMORY.md、YYYY-MM-DD.md）
```

**为什么**：(1) AI 的输入是文本——Markdown 文件不需要格式转换就能注入 systemPrompt；(2) 文件可以直接 git 版本控制；(3) 记忆规模很小（30K 字符上限），不需要 SQL 查询。代价是搜索只能按文件名和内容正则匹配，不能模糊语义搜索——但对于项目规模的记忆量，这完全够用。

### 决策 7：为什么选择 QProcess 而不是 std::system() 或 Win32 CreateProcess？

```
替代方案：std::system()、Win32 CreateProcessW()
实际选择：QProcess（Qt 封装）
```

**为什么**：QProcess 提供统一的跨平台接口（Windows 用 cmd.exe / Linux 用 /bin/sh），自动管理 stdin/stdout/stderr 分离，自带超时和 waitForFinished。std::system() 无法超时控制，CreateProcessW() 需要手写管道重定向。唯一代价是依赖 QtCore，但项目本身就用 Qt，这个代价为零。

### 决策 8：为什么桌面操作用 Win32 而不是 Qt 的跨平台 API？

```
替代方案：QCursor::setPos() + QTest::mouseClick() + QTest::keyClicks()
实际选择：SetCursorPos + SendInput(INPUT_MOUSE/INPUT_KEYBOARD)
```

**为什么**：QTest 是测试框架的 API，不是为生产环境设计的——它只在窗口拥有焦点时可靠。SendInput 是 Windows 最底层的输入注入 API，绕过了窗口焦点限制，可以模拟全局键盘鼠标操作。代价是代码绑定 Windows 平台，但桌面 Agent 本身就是 Windows-only 的定位。

### 决策 9：为什么 V19 用 Python sidecar，而不是把 Agent 主循环迁移到 Python？

```
替代方案：继续纯 C++；或直接用 Python/LangChain 重写 Agent
实际选择：C++ 保留主控，Python 作为能力层 sidecar
```

**为什么**：C++ 已经沉淀了 Qt UI、Agent 循环、工具注册、Win32 桌面操作、权限边界和 68 项测试。如果把 Agent 主循环迁移到 Python，会丢掉这些工程资产，并且形成两套 Agent 流程。Python 的优势在 AI 生态：多厂商 SDK、tokenizer、embedding、网页解析、文档解析、Playwright。因此更合理的边界是：C++ 负责“决策和权限”，Python 负责“模型和能力”。

**面试表达**：这是 sidecar 架构。主进程通过 `QProcess + JSONL` 调用 Python 子进程，既保持桌面主程序稳定，又为后续接入 Python AI 生态留下空间。

---

## 代码阅读指南

> 按这个顺序看源码，从外到内、从简到繁。

### 第一轮：建立地图（30 分钟）

| 顺序 | 文件 | 看什么 | 预期理解 |
|------|------|--------|---------|
| 1 | `src/core/AppConfig.h` | 配置结构体 | 项目"输入参数"长什么样 |
| 2 | `src/core/ChatMessage.h` + `MessageRole.h` | 消息模型 | 数据怎么在系统里流动 |
| 3 | `src/app/ApplicationController.h` | 公开接口 | 控制层对外能做什么 |
| 4 | `CMakeLists.txt`（根目录） | 子目录列表 | 项目有哪些模块 |

### 第二轮：跟踪一条消息（1 小时）

从用户点击发送到 AI 回复显示，跟踪调用链：

```
MainWindow::sendCurrentMessage()
  → ApplicationController::sendMessage()
    → SessionCoordinator::addUserMessage()
    → OpenAIClient::sendChat()
      → StreamParser::consume()       // SSE 解析
    → ApplicationController::handleTextDelta()  // 流式更新
    → MainWindow::assistantMessageUpdated()    // UI 刷新
    → SessionCoordinator::saveCurrentSession() // 持久化
```

### 第三轮：理解 Agent（1.5 小时）

从用户输入目标到自动执行完毕的完整路径：

```
ApplicationController::sendAgentLoopMessage()
  → AgentOrchestrator::startAgentLoop()
    → buildNextLoopPrompt()
      → classifyGoal()                 // 意图检测
      → reorderToolsByIntent()         // ⭐ 排序
      → buildToolGuidance()            // 最佳实践
    → OpenAICompatibleClient::sendChatWithTools()  // 带 tools schema
    → StreamParser → tool_calls 聚合
    → handleUnifiedAgentResponse()
      → AgentPlanParser::parse()       // 解析计划
      → AgentOrchestrator::executePlanAndReportToChat()
        → AgentToolRegistry::execute() // lambda 执行工具
        → AutoFix: cmake + ctest       // 自动验证
```

### 第四轮：深入一个子系统（1 小时）

选一个感兴趣的深入：

| 子系统 | 入口文件 | 关键逻辑 |
|--------|---------|---------|
| 工具注册 | `AgentToolRegistry.cpp` | `registerFileTools()`→`registerCommandTools()`→... 看一个工具是怎么注册的 |
| 记忆系统 | `ProjectMemoryManager.cpp` | `buildMemorySection()` 看三层记忆怎么拼接 |
| 桌面输入 | `InputSimulator.cpp` | `sendText()`→`mouseClick()`→`keyPress()` 看 SendInput 细节 |
| 技能匹配 | `SkillManager.cpp` | `matchSkills()` 看触发词怎么命中 |
| 上下文管理 | `ContextWindowManager.cpp` | 超 85% 阈值时怎么压缩 |
| Python 能力层 | `PythonSidecarClient.cpp` + `python/agent_sidecar/protocol.py` | `QProcess + JSONL` 如何把 C++ 主控和 Python 能力解耦 |

### 关键入口速查表

| 你想了解 | 从这里开始 |
|---------|-----------|
| 项目怎么启动 | `src/main.cpp` → `MainWindow` 构造 |
| 控制层怎么初始化 | `ApplicationController::initialize()` |
| 一次 Chat 请求怎么发 | `ApplicationController::sendMessage()` |
| 一次 Agent 请求怎么发 | `ApplicationController::sendAgentLoopMessage()` |
| 一个工具怎么注册 | `AgentToolRegistry::defaultRegistry()` |
| Agent 循环怎么跑 | `AgentOrchestrator::buildNextLoopPrompt()` |
| 记忆怎么注入 | `ProjectMemoryManager::buildMemorySection()` |
| Skill 怎么匹配 | `SkillManager::matchSkills()` |
| 文件怎么精确编辑 | `FileInteractionService::editTextFile()` |
| 截图怎么工作 | `ScreenCaptureService::captureToFile()` |
| 鼠标怎么点击 | `InputSimulator::mouseClick()` |
| 上下文超了怎么办 | `ContextWindowManager::manageContext()` |
