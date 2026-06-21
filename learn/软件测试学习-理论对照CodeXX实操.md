# 软件测试学习 — CodeXX 实操对照

> 左栏：《软件测试完全指南》的概念  
> 右栏：CodeXX 项目里对应的真实测试  
> 方法：两边并排读，跑 `cd build && ctest` 验证

---

## 第二章 基础概念映射

| 概念 | CodeXX 对��� | 具体代码 |
|------|-------------|---------|
| **测试用例** | 一个 `assert()` | `CronParserTest.cpp` 里每行 `assert()` 就是一个 case |
| **测试套件** | 一个测试 exe | `CronParserTest.exe` 本身就是一个套件（含 8 个断言） |
| **测试覆盖率** | CTest 63 个测试目标 | `ctest -N` 列出全部 |
| **回归测试** | 每次改完代码跑 `ctest` | 确保 63 个全绿 |
| **冒烟测试** | `MainWindowSmokeTest` | 只测构造/析构不崩溃 |

---

## 第三章 测试分类映射

| 分类 | 书中定义 | CodeXX 最典型的例子 |
|------|---------|-------------------|
| **单元测试** | 测试最小可测试单元 | `CronParserTest` — 只测 `nextRunTime()` 一个函数 |
| **集成测试** | 验证模块间交互 | `AgentLoopExecutionTest` — 涉及 AC + AIClient + Registry + Session |
| **黑白盒** | 不看/看源码 | 黑盒：`CronParserTest`(只看输入输出) / 白盒：`CodeHighlighterTest`(验证内部 format) |
| **手工测试** | 人工执行 | 启动 exe，手动聊两句验证 UI |
| **自动化测试** | CTest 自动跑 | `ctest` 一键跑 63 个 |
| **功能测试** | 测功能正确性 | `StreamParserTest` — 验证 SSE 解析逻辑 |
| **性能测试** | 测响应时间等 | ❌ CodeXX 暂无 |
| **冒烟测试** | 快速验核心功能 | `AppLaunchSmokeTest` — 启动应用不崩 |
| **Alpha/Beta** | 内部/外部测 | Alpha：你自己跑 Agent 循环测试 |

---

## 第四章 测试流程对照

| 流程阶段 | 书中描述 | CodeXX 实践 |
|---------|---------|-----------|
| **测试计划** | 分析需求、制定策略 | `docs/` 中的 PRD 文档，如 `47-v16-experience-enhancement-plan.md` |
| **测试设计** | 编写测试用例 | 看 `tests/CMakeLists.txt` 每个测试目标的设计意图 |
| **测试执行** | 跑测试 | `cd build && ctest --output-on-failure` |
| **缺陷管理** | 提交跟踪 | `git commit` + 修复后重跑 `ctest` |
| **测试报告** | `ctest` 输出 | `100% tests passed, 0 tests failed out of 63` |

---

## 第五章 测试用例设计方法 — 实操对照

### 5.1 等价类划分

**定义**：将输入分成若干等价类，每类取一个代表值。

**CodeXX 实例** — `CronParserTest`:

```cpp
// 等价类1: 有效cron
assert(nextRunTime("0 9 * * *", now).isValid());      // 代表：每天9点

// 等价类2: 无效cron
assert(!nextRunTime("abc def ghi", now).isValid());    // 代表：乱输入
assert(!nextRunTime("", now).isValid());               // 代表：空字符串
```

### 5.2 边界值分析

**定义**：大量缺陷在边界附近，测试最小值、最大值、刚好超出。

**CodeXX 实例** — `CodeHighlighterTest`:

```cpp
// 边界：空文档不崩溃
CodeHighlighter hl("cpp", new QTextDocument(""));
// 边界：不存在的语言
CodeHighlighter hl("nonexistent_lang", new QTextDocument(""));
// 边界：零行代码
CodeHighlighter hl("python", new QTextDocument(""));
```

**CodeXX 实例** — `TaskSchedulerTest`:

```cpp
// 边界：cron每个字段的最大值
assert(nextRunTime("59 23 31 12 6", now).isValid());  // 全最大值
// 边界：超出范围
assert(!nextRunTime("60 * * * *", now).isValid());     // 分钟=60 越界
```

### 5.3 场景法

**定义**：模拟用户操作流程设计用例。

**CodeXX 实例** — `AgentLoopExecutionTest`:

```cpp
// 场景：用户输入"帮我检查项目"
// → AC启动Agent循环
// → AI返回tool_call: read_text
// → 执行工具 → 追加observation
// → AI继续或结束
// → 验证最终session.messages数量 > 2
```

### 5.4 错误推测法

**定义**：基于经验推测容易出错的位置。

**CodeXX 实例** — `McpConnectorTest`:

```cpp
// 经验：网络/进程类操作容易出错
assert(!connector.connectToServer("", {}));             // 空命令 → false
assert(!connector.connectToServer("nonexistent", {}));  // 不存在 → false
assert(!connector.isConnected());                       // 未连 → false
assert(connector.listTools().isEmpty());                // 未连 → 空
```

---

## 第六章 自动化测试对照

| 书中概念 | CodeXX 现状 |
|---------|-----------|
| **回归测试自动化** | ✅ `ctest` 63 个全部自动化 |
| **CI/CD 流水线** | ❌ 暂无 Jenkins/GitHub Actions 集成 |
| **数据驱动测试** | ❌ 测试数据写死在代码里 |
| **模块化框架** | ⚠️ 每个测试是独立的 exe，但 `tests/CMakeLists.txt` 用变量共享源码路径 |
| **测试金字塔** | ✅ 大量单元测试(底部) + 少量集成测试(顶端) |

---

## 第七章 性能测试对照

| 概念 | CodeXX 现状 |
|------|-----------|
| 负载测试 | ❌ 无 |
| 压力测试 | ❌ 无（但 Agent 循环上限 50 轮算一种"自我保护"） |
| 关键指标 | TokenBar 可视为一种"延迟感知"——token 越多=请求越大=越慢 |

---

## 第八章 测试管理对照

| 概念 | CodeXX 实践 |
|------|-----------|
| 缺陷严重级别 | ❌ 无正式跟踪（但 `git log` + commit message 可回溯） |
| 测试覆盖率 | 没有量化统计，但每个模块都有对应的 test exe |
| 测试执行进度 | `ctest` 输出 `X/63 passed` |

---

## 第九章 工具对照

| 书中工具 | CodeXX 对应 |
|---------|-----------|
| JUnit / pytest / Google Test | 自研 `assert()` 轻量框架（零依赖） |
| Selenium / Playwright | ❌ 无 Web UI 测试 |
| JMeter | ❌ 无性能测试 |
| SonarQube | ❌ 无静态分析 |
| Jenkins | ❌ 无 CI（手动 `cmake --build` + `ctest`） |
| Postman | `OpenAICompatibleClient` 直接发 HTTP 请求 |

---

## 学习路线：从理论到实操

按这个顺序并排读书和代码，预计总共 4-6 小时：

```
第1小时：基础概念 (第2章)
  读：测试用例、测试套件、回归、冒烟的定义
  看：CronParserTest.cpp → 最干净的单元测试（8 行断言，零依赖）
  跑：cd build && ctest -R CronParserTest
  问：每行 assert 对应"等价类"还是"边界值"？

第2小时：测试分类 (第3章)
  读：单元/集成/系统/黑盒/白盒
  看：CodeHighlighterTest.cpp → 白盒测试（验证 format 内部状态）
  看：AgentLoopExecutionTest.cpp → 集成测试（多模块协作）
  跑：ctest -R Highlighter && ctest -R AgentLoop

第3小时：用例设计方法 (第5章)
  读：等价类、边界值、场景法、错误推测
  看：StreamParserTest.cpp → 验证 SSE 协议各种边界
  看：McpConnectorTest.cpp → 进程通信的错误推测
  动手：给 CronParser 加一个边界测试（比如"分钟=-1"）

第4小时：自动化测试 (第6章)
  读：测试金字塔、自动化适用场景
  看：tests/CMakeLists.txt → 理解 63 个测试如何组织
  跑：ctest --output-on-failure → 看失败时输出什么
  动手：给 MessageWidget 的代码块渲染写一个新 assert
```

---

## 进阶：你能直接在 CodeXX 上实践的事

| 练习 | 难度 | 做什么 |
|------|:---:|------|
| 给 CronParser 加边界测试 | ⭐ | 加"分钟=60""小时=25""秒级精度"的测试 |
| 给 CodeHighlighter 加新语言 | ⭐⭐ | 加 rust/go 语法测试，然后实现着色 |
| 给 StreamParser 写 mock 测试 | ⭐⭐⭐ | 不连 AI，用预设数据测解析器 |
| 给 AgentLoop 写完整集成测试 | ⭐⭐⭐⭐ | Mock AIClient，验证 3 轮循环的完整状态 |
| 用 Catch2 替换 assert() | ⭐⭐⭐ | 引入 `catch2.hpp`，改写一个测试看看效果 |
