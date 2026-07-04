# 面试问题预测与参考答案

本文列出围绕本项目可能被问到的问题，以及可以参考的回答。建议不要逐字背诵，而是理解后用自己的话表达。

## 1. 你这个项目是做什么的？

参考回答：

> 这是一个基于 C++17、Qt 6 和 CMake 的 Windows 桌面 AI 聊天应用。用户可以配置 DeepSeek 或 OpenAI 兼容接口的 Base URL、模型和 API Key，然后进行多轮对话。项目支持流式回复、多会话管理、角色提示词、Markdown 展示、会话导出、错误分类重试、应用内日志查看、本地文本工具、受控文件工具、会话收藏和归档。它同时也是我练习完整软件开发流程的项目，从需求、设计、任务拆分到测试、验收和发布都有文档记录。

## 2. 为什么选择 C++ 和 Qt？

参考回答：

> 这个项目定位是 Windows 桌面应用，Qt 在 C++ 桌面 GUI、网络、SQL、配置和打包方面都比较成熟。Qt Widgets 可以快速实现传统桌面界面，Qt Network 可以处理异步 HTTP 请求，Qt SQL 可以接 SQLite，整体技术栈比较统一。对我来说，它也适合练习 C++ 工程化开发，而不只是写算法代码。

## 3. 项目架构怎么设计？

参考回答：

> 我把项目分为 `ui`、`app`、`services`、`storage`、`tools`、`core` 和 `support`。`ui` 负责界面，`app` 里的 `ApplicationController` 负责业务流程，`services` 负责 OpenAI 兼容 API 和流式解析，`storage` 负责配置、凭据、聊天记录和模板存储，`tools` 负责本地文本处理工具，`core` 保存核心数据模型，`support` 放日志等通用工具。这样 UI 不直接操作网络和数据库，工具逻辑也不堆在主窗口里，后续功能扩展和测试都更容易。

## 4. ApplicationController 的作用是什么？

参考回答：

> `ApplicationController` 是界面和底层能力之间的协调层。比如发送消息时，它负责检查配置、更新当前会话、通知界面添加消息、调用 AI 客户端、接收流式文本、处理失败重试、保存聊天记录。如果这些逻辑都写在 `MainWindow`，界面类会越来越大，也不容易测试。抽出控制层后，界面更专注于展示和交互。

## 5. 你怎么实现流式回复？

参考回答：

> 网络层使用 `QNetworkAccessManager` 发起 POST 请求，并通过 `QNetworkReply::readyRead` 接收服务端分批返回的数据。OpenAI 兼容接口的流式响应是 SSE 格式，所以我单独写了 `StreamParser` 解析 `data:` 事件，从 JSON 里提取 delta content，并识别 `[DONE]`。控制层收到文本增量后，通知界面更新最后一条 AI 消息。

## 6. 为什么要单独写 StreamParser？

参考回答：

> 因为网络数据可能不是按完整消息边界到达的，一次 `readyRead` 可能只有半条 SSE 数据，也可能包含多条。把解析器单独抽出来，可以集中处理缓冲、切分、JSON 解析和完成标记，网络客户端只负责收发数据。同时 `StreamParser` 可以独立写单元测试，避免只能靠真实 API 手工测。

## 7. API Key 是怎么保存的？

参考回答：

> V3 之前 API Key 作为普通配置保存，后来我改成使用 Windows Credential Manager。项目里有一个 `CredentialStorage` 抽象，Windows 下的实现是 `WindowsCredentialStorage`，内部调用 `CredWriteW`、`CredReadW` 和 `CredDeleteW`。非敏感配置仍然保存在 `QSettings`，例如 Base URL、模型名和语言。这样可以把普通配置和敏感凭据分开。

## 8. 为什么不继续用 QSettings 保存 API Key？

参考回答：

> `QSettings` 适合保存普通配置，但 API Key 属于敏感信息，用普通配置保存容易被直接读取。Windows Credential Manager 是系统级凭据存储，更适合保存用户级密钥。项目当前只考虑 Windows，所以直接使用系统能力比引入跨平台库更简单。

## 9. 旧版本已经保存的 API Key 怎么处理？

参考回答：

> `ConfigStorage` 加了兼容迁移逻辑。加载配置时，先从 Credential Manager 读取 API Key。如果没有读到，再检查旧 `QSettings` 的 `api/apiKey`。如果旧值存在，就尝试写入 Credential Manager，写入成功后删除旧配置项。这个路径有自动化测试覆盖。

## 10. 聊天记录怎么保存？

参考回答：

> 聊天记录保存在本机 SQLite。`sessions` 表保存会话 ID、标题、角色提示词、创建时间、更新时间、收藏状态和归档状态，`messages` 表保存消息 ID、会话 ID、角色、内容和时间。这样可以支持多会话、搜索、收藏、归档、删除和导出。Qt 侧使用 `QSqlDatabase` 和 `QSqlQuery` 操作数据库。

## 11. 为什么用 SQLite？

参考回答：

> 桌面应用通常需要本地持久化，但不适合要求用户安装数据库服务。SQLite 是单文件数据库，部署简单，足够支持本地会话和消息管理。相比直接写 JSON 文件，SQLite 更适合做搜索、按更新时间排序和按会话 ID 查询消息。

## 12. 会话收藏和归档怎么做的？

参考回答：

> 我在 `ChatSession` 和 SQLite 的 `sessions` 表中增加了 `isFavorite` 和 `isArchived` 状态。默认列表只显示未归档会话，收藏筛选显示未归档且已收藏的会话，归档筛选只显示归档会话。排序仍然只按 `updatedAt` 倒序，收藏不会置顶，这样可以避免点击或标记会话时列表顺序变乱。旧数据库启动时会自动补列，默认值是未收藏、未归档。

## 13. 会话搜索怎么做的？

参考回答：

> 当前实现是 SQLite 查询会话标题和消息内容，使用 `LIKE` 做简单匹配。V5 中搜索会遵守当前筛选条件，比如在归档视图里只搜归档会话。对于个人项目和中小规模数据，这样足够直接。后续如果聊天记录很多，可以升级到 SQLite FTS 全文索引，提高搜索性能和匹配能力。

## 14. 你修过哪些比较典型的 bug？

参考回答：

> 一个是点击会话导致列表顺序变乱。原因是切换会话和更新会话都触发了保存和置顶。后来我区分了“查看”和“内容更新”，只有真正更新内容时才移动到顶部。另一个是关闭窗口后进程仍占用 exe，后来在 `closeEvent` 里取消正在进行的请求并明确退出 Qt 事件循环。

## 15. 停止生成怎么实现？

参考回答：

> 生成中点击发送按钮会变成停止逻辑。控制层调用 AI 客户端的 `cancel()`，客户端 abort 当前 `QNetworkReply` 并清理状态。控制层把空回复更新为“已停止”，然后保存会话。这样用户不需要等长回复结束。

## 16. 错误分类和重试怎么实现？

参考回答：

> 服务层根据 HTTP 状态码和网络错误映射到 `RequestErrorCategory`，例如 401/403 是认证错误，429 是额度或频率限制，400/404/422 可能是模型或参数错误，5xx 是服务端错误。控制层根据分类生成更友好的提示。失败后会记录上一条用户消息，界面显示重试按钮，点击后移除失败回复并重新发送上一条用户消息。

## 17. 为什么重试时要删除失败回复？

参考回答：

> 如果不删除失败回复，下一次请求可能把错误提示也作为上下文发给模型，污染对话上下文。重试应该表示“对同一条用户消息再请求一次”，所以要移除上一次失败产生的 AI 消息。存储层也要同步替换消息列表，避免数据库里旧失败回复重新出现。

## 18. 日志如何避免泄露敏感信息？

参考回答：

> 日志只记录请求开始、完成、取消、失败分类等信息，不记录请求体和聊天正文。`AppLogger` 会对 Bearer token、apiKey 这类字段做脱敏替换。应用内日志窗口只是读取日志文件最近内容，不额外输出敏感信息。

## 19. 你做了哪些测试？

参考回答：

> 项目使用 CTest 统一运行测试。测试覆盖核心模型、服务商预设、会话列表排序、SSE 解析、请求体构建、HTTP 错误分类、配置和凭据迁移、SQLite 存储、Markdown 导出、日志脱敏、日志读取、本地工具逻辑、Agent 工具目录、计划解析、Agentic Loop、工具注册表、Function Calling schema、tool_calls 流式解析、命令执行策略、记忆/Skills/Hooks/MCP、UI smoke test，以及 V19 Python sidecar 协议和 QProcess 客户端。当前是 69 个测试全部通过。

## 20. UI 部分怎么测试？

参考回答：

> 当前 UI 主要是 smoke test，比如设置窗口、角色提示词窗口、工具窗口、消息复制和 Markdown 展示。完整 UI 自动化测试还没有接入，这是项目的后续优化方向。核心策略是把能独立测试的逻辑尽量抽到非 UI 模块，例如 SSE 解析、存储、导出、日志读取和本地工具逻辑。

## 21. 项目是怎么打包的？

参考回答：

> 使用 CMake 配置 Release 构建，生成 `AIChatDesktop.exe` 后，用 Qt 的 `windeployqt` 收集 Qt 运行依赖、平台插件、SQL 驱动和 TLS 插件。最后把发布目录压缩成 zip。v1.0 已生成 `AIChatDesktop-1.0-windows.zip`，并做过发布目录启动关闭 smoke check。

## 22. 如果让你继续优化，你会做什么？

参考回答：

> 我会优先做三件事。第一是把会话管理继续增强，比如标签和 SQLite FTS 全文搜索。第二是进入 AI 任务拆解和受控工具建议，让 AI 先能规划步骤但仍由用户确认执行。第三是继续补 UI 自动化测试，覆盖设置、发送消息、工具窗口和日志窗口等关键流程。

## 23. 文件工具怎么控制风险？

参考回答：

> 文件工具没有让 AI 自动操作电脑，而是要求用户通过文件选择框明确选择路径。读取文本文件有大小限制，也会拒绝疑似二进制文件；保存已有文件必须二次确认；打开文件或文件夹前也会确认。日志只记录操作类型、结果、输出长度和路径摘要，不记录文件正文或完整目录。

## 24. 什么时候适合做 AI Agent 执行命令？

参考回答：

> 我会放在 V9，而不是现在直接做。V6 先完成受控文件工具，V7 让 AI 做任务拆解和工具建议，V8 先做默认工作目录内的文件 Agent，让文件生成、读取、覆盖、删除和连续执行边界稳定。命令执行必须等这些能力稳定后再进入，并限制在白名单、固定工作目录、执行前确认、超时控制和日志审计内，不能直接执行任意 PowerShell 字符串。

## 25. V7 Agent 怎么避免 AI 直接操作电脑？

参考回答：

> V7 不是让 AI 自动控制电脑，而是让 AI 返回结构化 JSON 计划。计划先经过本地解析器校验，工具 ID 必须存在于工具目录，风险等级以本地目录为准。用户在计划窗口确认后，才会执行低风险文本工具。文件读取这类中风险能力仍走文件选择框，不允许 AI 从参数里直接指定路径执行。

## 26. 模拟键盘鼠标操作应该怎么做？

参考回答：

> 这类能力应该更靠后，比如 V11。第一版不建议直接录制全局键盘输入，而是优先记录应用内工具步骤。真正做设备输入模拟时，应该优先用 Windows UI Automation 按控件操作，键鼠事件只作为 fallback，并且要有前台窗口校验、停止按钮、执行前确认，不能自动输入密码、API Key 或验证码。

## 27. 项目中你最满意的设计是什么？

参考回答：

> 我比较满意的是分层和安全存储这两部分。分层让 UI、控制层、服务层和存储层职责比较清楚，后续加重试、日志查看、模型参数时没有把主窗口写得特别混乱。安全存储则体现了真实项目里对敏感信息的考虑，不只是把功能跑通。

## 28. 项目中最大的不足是什么？

参考回答：

> 最大不足是 UI 自动化测试还不完整，界面交互主要依赖 smoke test 和人工验收。另外 Markdown 渲染只是基础能力，代码高亮和复杂表格还没做。会话搜索现在用 LIKE，数据量大时性能也需要优化。

## 29. 你在这个项目中学到了什么？

参考回答：

> 我学到的不只是 Qt API，而是完整开发流程。包括如何从需求拆任务，如何做分层设计，如何用 Git 分支和 PR 管理迭代，如何写验收文档，如何用自动化测试保护核心逻辑，以及如何考虑安全、日志、发布这些非功能需求。

## 30. 如果面试官问“这是不是只是调用 API？”

参考回答：

> 调用 API 只是其中一部分。这个项目还包含桌面 GUI、流式响应解析、多会话本地存储、安全凭据保存、错误分类重试、日志脱敏、Markdown 导出、本地工具系统、会话收藏归档、Windows 打包和自动化测试。真正的工程价值在于把这些能力组织成一个可维护、可验证、可发布的桌面应用。

## 31. V8.1 工作目录 Agent 是怎么实现的？

参考回答：

> V8.1 是在 V7 结构化计划的基础上扩展的。AI 仍然只返回 JSON 计划，本地解析器校验工具 ID 和风险等级。新增的 `workspace.*` 工具会交给 `AgentPlanExecutor`，执行前先通过 `WorkspacePolicy` 判断路径是否在 Agent 工作目录内、是否是受保护文件、操作类型是否允许。真正文件读写由 `WorkspaceFileService` 完成。这样模型负责建议，本地代码负责权限和执行。

## 32. 为什么要限制 Agent 工作目录？

参考回答：

> 如果让 AI 直接使用任意绝对路径，风险会很高，比如误删项目文件、读取密钥或改系统目录。工作目录相当于一个本地沙盒，所有自动文件操作都必须落在这个目录内。相对路径会解析到工作目录，路径穿越和工作目录外绝对路径会被拒绝。这样可以让 Agent 具备生成文件能力，同时避免扩大到全系统自动化。

## 33. 覆盖和删除文件怎么降低风险？

参考回答：

> `workspace.write_text` 只创建新文件，不覆盖已有文件。真正覆盖必须使用 `workspace.overwrite_text`，并且覆盖前会生成 `.bak` 备份。删除也不是永久删除，而是把普通文件移动到工作目录内的 `.trash`。此外 `.git`、`.env`、凭据、密钥、证书等受保护文件禁止自动创建、覆盖和删除。

## 34. 怎么防提示词注入？

参考回答：

> 文件读取结果会被包装成 `UNTRUSTED WORKSPACE FILE DATA`，并明确说明里面的内容只能作为数据分析，不能作为命令执行。继续规划时也会追加同样的边界说明。更关键的是，本地执行器不会因为文件内容改变工具权限，所有路径和操作仍然由 `WorkspacePolicy` 决定。

## 35. Agentic Loop 是怎么做的？

参考回答：

> V8.2 新增了 `AgentLoopController`。它按观察、选择下一步、执行、评估的循环运行，每轮只执行一个工具步骤。它有最大步数、总耗时、单步耗时、停止请求、失败暂停和重复动作检测。计划窗口的连续执行已经接入这个控制器，所以后续接真实 AI 单步规划时，不需要重新写运行时安全逻辑。

## 36. ToolRegistry 解决了什么问题？

参考回答：

> 之前工具描述在 `AgentToolCatalog`，执行逻辑在 `AgentPlanExecutor`，容易出现 Prompt 里有工具但执行器没同步的问题。V8.3 把工具 ID、描述、风险等级、参数 schema、Function Calling 函数名和执行函数统一放进 `AgentToolRegistry`。现在工具目录从注册表生成，执行器也通过注册表执行，减少硬编码分支。

## 37. Function Calling 兼容层做到哪一步？

参考回答：

> 当前已经能从 `AgentToolRegistry` 生成 OpenAI-compatible `tools` schema，并把 `workspace.write_text` 这类工具 ID 转成 `workspace_write_text` 这样的函数名。V9.2 第一版已经把 Agent 计划请求切换为可携带原生 tools：服务层会解析流式 `tool_calls`，控制层再把函数名映射回本地工具 ID，生成计划预览。旧 JSON plan fallback 仍保留，如果模型不返回 tool calls 或服务商不兼容 tools 字段，仍能退回原流程。

## 38. V9 命令执行为什么不直接开放 PowerShell？

参考回答：

> 因为任意 PowerShell/CMD 字符串风险太高，模型可能拼出删除文件、修改系统配置或泄露敏感信息的命令。V9 只开放白名单模板，比如 `git status --short --branch`、`git diff --check`、`cmake --build build-qt` 和 `ctest --test-dir build-qt --output-on-failure`。本地 `CommandPolicy` 负责把工具 ID 映射成固定程序和参数数组，并校验工作目录；`CommandRunner` 用 `QProcess` 执行，不通过 shell 拼接，同时处理超时、输出截断和敏感字段脱敏。

## 39. 命令执行怎么降低误操作风险？

参考回答：

> 第一，命令必须来自本地工具注册表，AI 不能自定义程序和参数。第二，命令工作目录必须是安全项目目录，不能是磁盘根目录、系统目录或用户主目录根部。第三，命令有超时，非零退出码或超时都会让工具失败并暂停连续执行。第四，日志只记录模板 ID、退出码、输出长度和超时状态，不记录完整 stdout/stderr。第五，`git add`、`git commit`、`git push` 这类会改变远程或版本库状态的命令暂不开放。

## 40. V9.1 的开发者命令技能是什么？

参考回答：

> V9.1 把常见开发流程整理成静态技能目录，例如“检查当前改动”“提交前检查”“构建并测试”“定位测试失败”。技能本身不直接执行命令，而是展开为一组 `command.*` 工具步骤，比如提交前检查会展开为 `git diff --check`、`cmake --build build-qt` 和 `ctest --test-dir build-qt --output-on-failure`。真正执行时仍然经过工具注册表、命令策略、用户确认和命令运行器，所以技能不会绕过安全边界。

## 41. V10.1 的项目级指令怎么做？

参考回答：

> V10.1 支持读取 Agent 项目目录下的 `AGENT.md`。它可以记录项目技术栈、构建测试命令、Git 约定和当前阶段路线。读取后不会直接当成系统指令，而是由 `ProjectInstructionService` 包装成“不可覆盖安全规则的不可信项目上下文”，再交给 `AgentPlanPromptBuilder` 注入计划 Prompt。缺少文件时静默跳过，文件过大时只读取前 16 KB。这样能让 Agent 更理解项目约定，同时不扩大工具权限。

## 42. V10.2 的外部技能文件怎么做？

参考回答：

> V10.2 支持读取 Agent 项目目录下的 `skills/*.skill.md`。技能文件用 JSON 描述技能 ID、中英文名称、描述和步骤列表。读取后会先校验工具 ID 是否存在于 `AgentToolRegistry`，并且必须允许计划窗口直接执行；风险等级也会按本地工具目录提升。外部技能只会追加到内置技能后面，重复 ID 不会覆盖内置技能。它只是让模型更容易生成常见流程的计划，不会绕过计划预览、用户确认、命令白名单或本地安全策略。

## 43. V10.3 的工作记忆怎么控制风险？

参考回答：

> 记忆文件只作为项目上下文，不自动保存聊天全文或模型输出。追加记忆必须走用户确认流程。明显包含凭据或密钥的内容会被拒绝。记忆不能扩大工具权限或绕过安全策略。

---

## v1.0 新增面试问题（V12 - V18 Agent 系统）

## 44. 你的 Agent 系统是怎么工作的？

参考回答：

> 我实现了一个 Agentic Loop。用户输入目标后，系统先做意图分类——通过关键词把目标归到 7 种场景之一，比如改代码就是 CodeEdit。然后按意图把匹配的工具排到前面加 ⭐ 推荐，不相关的工具软限制 30 个。之后进入 OODA 循环：每一轮 AI 选择一个工具执行，执行完把结果追加到 observation，然后再让 AI 选择下一步。循环继续直到 AI 返回 done=true。过程中有重复动作检测、上下文压缩、输出截断续接等保护。

## 45. 51 个工具有哪些类别？怎么管理的？

参考回答：

> 分为 7 大类：文件操作（read/save/edit/copy/move/delete/grep/archive 等 15 个）、命令执行（bash/git/cmake/ctest 等 6 个）、桌面感知（截图/OCR/窗口枚举/焦点控件/屏幕尺寸等 9 个）、桌面操作（鼠标点击拖拽滚轮/键盘输入/按键等 8 个）、剪贴板（读写 2 个）、网络请求（HTTP 的 GET/POST/下载/打开 URL 共 4 个）、代码执行（Python/JS 沙箱和子代理探索 2 个），还有 JSON 格式化、项目文件搜索等辅助工具。

> 管理上用了统一注册表 AgentToolRegistry，每个工具一个条目包含 ID、描述、参数 schema、执行函数。新增工具只需要在注册表里加一个条目。7 个 CMake 子库按领域独立编译。

## 46. Function Calling 是怎么实现的？

参考回答：

> Qt 的 QNetworkAccessManager 发起 HTTP 请求时会附带 tools 数组——这个数组是从 AgentToolRegistry 的 tool descriptors 自动生成的 Function Calling schema。模型返回的 tool_calls 增量文本由 StreamParser 实时聚合，解析出 function name 和 arguments。function name 必须在注册表中存在，否则拒绝。arguments 是 JSON object，由 lambda 闭包接收后调用对应的 FileInteractionService 或 InputSimulator。

## 47. 桌面自动化具体怎么做？从截图到点击的全链路。

参考回答：

> 第一步 system.capture_screen 用 QScreen::grabWindow 截图存 PNG。第二步 system.ocr_text 用 Windows.Media.Ocr API 提取文字，获得坐标信息。第三步 system.active_control 用 UIAutomation COM 接口获取当前焦点控件名称和位置。第四步 input.validate_foreground 用 GetForegroundWindow 确认前台窗口正确。第五步 input.mouse_click 用 SetCursorPos + SendInput(INPUT_MOUSE) 点击目标坐标。第六步 input.type_text 用 SendInput(KEYEVENTF_UNICODE) 输入文本。整个过程有系统窗口黑名单保护，不能操作 Task Manager、UAC 弹窗、Ctrl+Alt+Del 屏幕。

## 48. 三层记忆系统是什么？怎么设计的？

参考回答：

> L1 是用户级跨项目偏好，保存在 ~/.codex/MEMORY.md。L2 是项目级技术决策和约定，保存在 .workbuddy/memory/MEMORY.md。L3 是每日工作日志，Agent 执行完自动追加到 YYYY-MM-DD.md。

> 设计上每一层独立读写。L3 有 14 天窗口、50 条上限、30K 字符硬上限。超过 14 天的日志进入 ISO 周压缩，通过 LLM 生成摘要保存为 compressed.md。所有三层记忆都通过 buildMemorySection() 拼接后注入到 systemPrompt。安全方面，包含 api_key、token、password 等敏感字段的内容会被自动拒绝。

## 49. Skill 系统是什么？和工具注册表有什么关系？

参考回答：

> 工具注册表告诉 AI "你能用什么"，Skill 告诉 AI "在这个场景下你应该怎么用"。Skill 是一个 YAML frontmatter + Markdown 指令的 SKILL.md 文件，放在项目的 .workbuddy/skills/ 目录。用户输入触发词匹配到某个 Skill 时，Skill 的指令会注入到 Agent 提示词的 [Active Skills] 段。

> 比如用户说"全自动开发：帮我改个 bug"，SkillManager 匹配到 auto-dev-cycle 这个 Skill，它的指令会告诉 AI：先读文件→修改→自动构建→测试→失败就修复→最终报告。Skill 只是工作流指导，不会绕过工具注册表的安全限制。

## 50. 怎么防止 Agent 陷入死循环？

参考回答：

> 有三层防护。第一层是重复动作检测：同一个 toolId + 相同参数出现 3 次就终止，维护了一个指纹表。第二层是上下文压缩：Microcompact 把早期 observation 压缩为短摘要（只保留最近 5 条完整），Reactive 压缩在 API 报 context_length_exceeded 时触发。第三层是 Stop Hook：在提示词中注入规则，要求 AI 在 done=true 前必须验证目标是否真的完成了。

## 51. 编辑代码后怎么自动验证？

参考回答：

> 我设计了一个 AutoFix 闭环。AgentOrchestrator 在执行完计划后，检测是否有 file.edit_text 或 workspace.write_text 这类编辑操作。如果有，自动用 QProcess 运行 cmake --build 和 ctest，把构建和测试结果作为 observation 注入到 Agent 循环中。如果构建或测试失败，AI 看到失败信息可以自动修复再重试。最多 5 次循环。

## 52. 项目从聊天客户端到 Agent 平台的架构演进是怎样的？

参考回答：

> V1-V11 是聊天客户端阶段，重点是流式对话、会话管理、安全存储这些基础能力。V12 开始逐步加入 Agent 循环、连续执行能力。V13 是最重要的一步——加入三层记忆系统、Skill 系统和 Hook 系统，让 Agent 有了"记忆"和"场景化行为"。V14-V15 加入桌面自动化和架构重构，ApplicationController 从一个 350 行的单体类拆分为三个 Coordinator。V18 是能力爆发期，工具从 20 多个扩展到 51 个，加上了意图感知工具排序、AutoFix、智能步骤折叠。

> 整个演进过程我是独立完成的。从"能聊天"到"能像人一样操作电脑"，每一步的架构决策都是基于前一步遇到的问题。

## 53. 为什么选 C++/Qt 做 Agent 而不是 Python？

参考回答：

> 功能上说，Python 的 langchain/autogen 确实生态更成熟，但这个项目的核心目标是桌面 Agent：Qt UI、Win32 SendInput、UIAutomation、Windows Credential Manager、工具权限边界这些能力更适合在 C++/Qt 主程序里掌控。后续我没有否定 Python，而是采用 sidecar 架构：C++ 保留 Agent 主循环和本机权限，Python 通过 JSONL 协议提供模型调用、tokenizer、Web/文档解析等能力。这样能同时保留 C++ 工程资产和 Python AI 生态。

## 54. 这个项目最大的技术难点是什么？

参考回答：

> 最难的倒不是某个单一技术点，而是系统复杂度的管理。从 20 个工具到 51 个工具，从 1 个 ApplicationController 到 3 个 Coordinator + 7 个工具子库，再到 V19 Python sidecar，每一步都要保证 69 个测试零回归。印象最深的是工具注册表的设计——一开始工具描述散落在 Prompt 构建、计划执行、Function Calling schema 三个地方，每加一个工具要改三处。后来我把它统一到 AgentToolRegistry，描述和执行函数来自同一份定义，新增工具只需要 10 行代码。这就是设计模式里说的"单一数据源"。

> V10.3 的记忆是项目级受控记忆，不是自动长期记忆。应用会读取项目目录下的 `AGENT_MEMORY.md` 作为受限上下文；追加记忆必须通过 `memory.append_project_note` 工具，并经过计划窗口和用户确认。服务层会拒绝明显包含 API Key、password、token、Bearer、secret 等敏感字段的内容，也限制单条记忆长度。记忆只帮助 Agent 理解用户明确要求保存的偏好或项目决策，不能扩大工具权限、绕过确认或覆盖本地安全策略。
