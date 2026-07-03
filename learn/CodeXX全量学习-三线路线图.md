# CodeXX 全量学习 — 开发/测试/运维三线路线图

> 结合真实项目源码与岗位知识体系，可学可练可面试

---

## 一、开发方向（C++/Qt 全栈）

### 1.1 构建系统与工程化

| 知识点 | 对应文件 | 做什么 |
|--------|---------|--------|
| CMake 子目录拆分、add_subdirectory | `CMakeLists.txt` + `src/*/CMakeLists.txt` | 理解 12 个静态库如何组织 |
| target_link_libraries 传递依赖 | `src/app/CMakeLists.txt` | `codexx_app` 链接 `codexx_ui` → 自动拉 `codexx_tools` |
| MinGW vs MSVC 编译差异 | `CMakeLists.txt:199-206` | MinGW 需 `-mwindows`、`WIN32_EXECUTABLE=FALSE` |
| CTest 集成 | `tests/CMakeLists.txt` | `add_test(NAME xxx COMMAND xxx)` |
| Debug vs Release 配置 | `build/CMakeCache.txt` | `CMAKE_BUILD_TYPE:STRING=Debug` |

### 1.2 Qt 框架核心

| 知识点 | 对应文件 | 做什么 |
|--------|---------|--------|
| 信号槽机制（核心概念） | `ApplicationController.h:122-152` | 30+ 信号 → MainWindow 槽连接 |
| QObject 生命周期 | `ApplicationController.cpp:51` | `~ApplicationController() override` 自动清理 unique_ptr |
| QMainWindow 布局 | `MainWindow.cpp:56-214` | 侧边栏(248px) + ChatView + Composer |
| QScrollArea + QVBoxLayout | `ChatView.cpp:10-32` | 聊天消息列表的滚动容器 |
| QLabel RichText vs PlainText | `MessageWidget.cpp:246-256` | Markdown 渲染通过 Qt::RichText 模式 |
| QPlainTextEdit 只读模式 | `MessageWidget.cpp:258-299` | 代码块独立渲染，NoWrap |
| QDialog + QFormLayout | `SettingsDialog.cpp:74-179` | 设置窗口的典型实现 |
| QSS 样式表 | `resources/styles/app.qss` | 813 行暗/亮双主题 |
| eventFilter 拦截事件 | `MainWindow.cpp` | Ctrl+V 粘贴检测 |
| QProcess 外部进程 | `McpConnector.cpp`, `CommandRunner.cpp` | MCP 协议、命令执行 |
| Python sidecar | `PythonSidecarClient.cpp`, `python/agent_sidecar` | V19 能力层，JSONL over QProcess |
| QSyntaxHighlighter | `CodeHighlighter.cpp` | 12 种语言语法着色 |

### 1.3 C++ 工程能力

| 知识点 | 对应文件 | 做什么 |
|--------|---------|--------|
| 智能指针 | `ApplicationController.h:228` | `std::unique_ptr<TaskScheduler>` |
| namespace 作为模块 | `AgentLoopController.h:92` | `namespace AgentLoopController {}` 替代 class |
| 匿名 namespace 封装内部函数 | `MessageWidget.cpp:16-141` | `splitAssistantContent()` 等 15 个内部函数 |
| std::function 回调 | `AgentLoopController.h:67-70` | `AgentLoopCallbacks { stepStarted, stepFinished }` |
| 工厂模式 | `ChatMessage.h:19-28` | `static ChatMessage::create()` |
| JSON 序列化/反序列化 | `AgentLoopState.h` | `toJson()/fromJson()` |
| QRegularExpression | `BuiltinHooks.cpp:102-129` | 敏感信息正则过滤 |
| 枚举 + switch 映射 | `AgentLoopController.cpp:240-262` | `runStatusToString()` |

### 1.4 HTTP 客户端与流式解析

| 知识点 | 对应文件 | 做什么 |
|--------|---------|--------|
| QNetworkAccessManager | `OpenAICompatibleClient.cpp:62` | POST 请求 |
| SSE 流式协议 | `StreamParser.cpp:58-63` | `data:` 行解析 |
| JSON 流式累积判断完整性 | `StreamParser.cpp:161-188` | `QJsonDocument::fromJson() != null` |
| OpenAI API 协议 | `buildRequestBody` | messages + tools + stream + tool_choice |
| 多模态图片格式 | `OpenAICompatibleClient.cpp:265-280` | base64 → `{type:"image", image:{data, format}}` |
| Python 能力层协议 | `PythonSidecarProtocol.cpp`, `protocol.py` | 一行一个 JSON 请求/响应，带 id/ok/error |

### 1.5 算法与数据结构

| 知识点 | 对应文件 | 做什么 |
|--------|---------|--------|
| Cron 表达式解析器 | `TaskScheduler.cpp:35-100` | 5 字段逐层匹配，支持 */, 范围 |
| Markdown 代码块拆分解 | `MessageWidget.cpp:84-133` | 逐行扫描 ``` 标记 |
| 表格行识别 | `MessageWidget.cpp` | `|`开头行→连续合并 |
| Token 估算算法 | `ApplicationController.cpp` | `(ASCII + CJK*2) / 3` |
| 上下文压缩策略 | `AgentLoopController.cpp:162-194` | 保留首尾，截断中间 |

### 1.6 建议学习顺序

```
第1周: CMake 静态库拆分 + 信号槽
第2周: QMainWindow 布局 + QSS 样式
第3周: HTTP 客户端 + SSE 流式解析
第4周: QProcess + Python sidecar + JSONL 协议
第5周: 智能指针 + 工厂模式 + JSON 序列化
```

---

## 二、测试方向（QA 工程师视角）

### 2.1 测试框架与组织

| 知识点 | 对应文件 | 做什么 |
|--------|---------|--------|
| CTest 注册与执行 | `tests/CMakeLists.txt` | `add_test(NAME ...)` 注册 68 个 |
| `assert()` 断言机制 | 每个测试 exe 的 `main()` | 失败→abort→退出码≠0→CTest FAILED |
| 测试金字塔 | 整个 tests/ | 45单元+8集成+2系统 |
| 每个测试 exe 链接最小静态库 | `tests/CMakeLists.txt` | CronParserTest 只链 `codexx_scheduler` |

### 2.2 测试用例设计方法（对照书中五章）

| 方法 | 对应测试 | 实操 |
|------|---------|------|
| **等价类划分** | `CronParserTest` | 有效cron类 | 无效cron类 |
| **边界值分析** | `CronParserTest`, `CodeHighlighterTest` | 分钟=0/59, 分钟=60; 空文档、未知语言 |
| **场景法** | `AgentLoopExecutionTest` | 模拟"用户输入→Agent循环→完成"全流程 |
| **错误推测法** | `McpConnectorTest` | 空命令、不存在命令、断开状态 |
| **断言覆盖** | `AgentPlanPromptBuilderTest` | 18条 assert 覆盖 4 个场景 |

### 2.3 测试分类实战

| 类型 | 定义 | 要看的具体测试 |
|------|------|--------------|
| **单元测试** | 测一个函数，零依赖 | `CronParserTest` — 8 行断言 |
| **集成测试** | 测多模块协作 | `AgentLoopExecutionTest` — AC+Orch+Registry+Session |
| **系统测试** | 测完整 exe | `AppLaunchSmokeTest` — 启动 AIChatDesktop.exe |
| **冒烟测试** | 测核心功能快速验证 | `MainWindowSmokeTest` — 构造不崩 |
| **回归测试** | 改代码后重跑 | `ctest` 一键全跑 |
| **黑盒测试** | 不看源码只看输入输出 | CronParserTest (不管你内部怎么算) |
| **白盒测试** | 看源码验证内部状态 | CodeHighlighterTest (验证 format 颜色) |

### 2.4 测试质量的评估维度

| 维度 | 怎么评估 | CodeXX 现状 |
|------|---------|-----------|
| **测试覆盖率** | 每个模块有对应 test 吗？ | 12 个 src 子目录都有对应测试 |
| **断言密度** | 每个测试 exe 平均多少条 assert？ | 约 8-15 条 |
| **回归频率** | 每次提交跑测试吗？ | 手动 `ctest`，无 CI 自动触发 |
| **缺陷发现率** | 测试抓到过真实 bug 吗？ | 多次（如文件工具删除崩溃、暗色主题代码块不可读） |

### 2.5 建议学习顺序

```
第1天: 看懂 5 个典型测试的 assert 逻辑
第2天: 对比书的等价类/边界值/场景法章节 + 对应的测试源码
第3天: 给 CronParserTest 加 3 条新 assert
第4天: 自己写一个"引用的测试文件"
```

---

## 三、运维方向（DevOps/SRE 视角）

### 3.1 构建与部署

| 知识点 | 实际操作 |
|--------|---------|
| CMake 多构建目录 | 项目有 4 个 build 目录：`build/`, `build-qt/`, `build-release/`, `build-release-qt/` |
| Release 打包 | `release/` 目录含 exe + 28 个 dll + 32 个 qm 语言包 |
| 多编译器切换 | MinGW GCC 15.2（当前）、MSVC（历史） |
| 依赖管理 | Qt6 的 Core/Widgets/Network/Sql/Test 5 个模块 |

### 3.2 CI/CD 与自动化

| 知识点 | CodeXX 现状 | 可以学什么 |
|--------|:---:|------|
| 持续集成 | ❌ 无 CI | 学习如何加 GitHub Actions / Jenkins |
| 持续部署 | ❌ 手动 | 学习如何加自动打包发布 |
| 自动化测试流水线 | 手动 `ctest` | 学习如何在 CI 中触发 `ctest --output-on-failure` |
| 版本号管理 | `CMakeLists.txt` 中 `VERSION 1.0` | 学习语义化版本 |

### 3.3 日志与监控

| 知识点 | 对应文件 | 做什么 |
|--------|---------|--------|
| 应用日志 | `AppLogger` | Agent 每步执行都写日志 |
| 日志查看器 | `LogViewerDialog` | 内置 UI 查看日志 |
| 错误恢复 | `ApplicationController::handleRequestFailed()` | 上下文溢出→压缩→截断→最小上下文 |
| Agent 状态持久化 | `AgentLoopState.h` → `agent_state.json` | 崩溃恢复 |

### 3.4 性能与安全

| 知识点 | 具体内容 |
|--------|---------|
| Token 消耗监控 | `TokenBar` 实时显示 token 用量 |
| 内存管理 | 智能指针 `unique_ptr` 自动释放 |
| API Key 过滤 | `SensitiveFilterHook` 正则替换泄露凭据 |
| 速率限制 | `RateLimitHook` 60 秒内最多 20 次 |
| 命令安全策略 | `CommandPolicy` 允许的命令模板白名单 |
| 系统窗口保护 | `isSystemProtected()` 黑名单防操作 |

### 3.5 建议学习顺序

```
第1天: 看懂 CMakeLists.txt 的 12 个静态库拆分
第2天: 手动编译 build/ 和 release/ 两个版本
第3天: 给项目写一个 .github/workflows/ci.yml（CI 自动跑 ctest）
第4天: 学习 AppLogger 的日志格式 + 敏感信息过滤
```

---

## 四、按岗位对照

| 岗位 | 核心技能 | CodeXX 知识点 | 可写到简历上的话 |
|------|---------|-------------|---------------|
| **C++ 开发** | Qt6、CMake | 12 静态库架构、信号槽 | "参与 Qt6/C++ AI 桌面应用开发，实现 Agent 循环和 30+ 工具系统" |
| **QA 测试** | 单元/集成/系统测试 | 68 个 CTest | "从零建立自动化测试体系，覆盖 68 个测试用例，使用 assert 框架" |
| **DevOps** | CI/CD、打包 | 4 个 build 目录 | "配置 CMake 多编译器构建，实现 Debug/Release 自动化打包" |
| **AI 应用开发** | Function Calling、Agent | Agent 循环全流程 | "实现 OpenAI-compatible 的 Agent 循环，支持 50 轮自主决策" |
| **安全工程师** | 注入防护、凭证过滤 | Hook 系统 | "实现 4 层注入防护：prompt 围栏+敏感过滤+截断+系统保护" |

---

## 五、我该从哪开始？

| 你的目标 | 先看这个 | 用时 |
|---------|---------|:---:|
| 理解项目全貌 | `docs/架构优化方向.md` | 15 分钟 |
| 学 CMake 工程化 | `CMakeLists.txt` + `src/*/CMakeLists.txt` | 1 小时 |
| 学 Qt 信号槽 | `ApplicationController.h` 的 signals 区域 → `MainWindow.cpp` 的 connectController | 1 小时 |
| 学测试 | `learn/软件测试学习-理论对照CodeXX实操.md` | 4 小时（含动手） |
| 学 Agent 循环 | `AgentLoopPromptBuilder.cpp` → `AgentOrchestrator.cpp` → `ApplicationController.cpp:handleRequestFinished` | 2 小时 |
| 学 Python 能力层 | `learn/07-Python能力层学习.md` | 1 小时 |
| 面试准备 | 上面 5 个都过一遍，挑 2 个深入 | 1 天 |
