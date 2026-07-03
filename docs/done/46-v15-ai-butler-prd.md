# V15 个人 AI 管家整合 — 产品需求文档

**文档状态**: 待评审
**创建日期**: 2026-06-04
**关联版本**: CodeXX V15
**前置依赖**: V12 (Agent 基础设施), V13 (记忆+技能), V14 (感知-操作闭环)

---

## 1. 项目信息

| 字段 | 值 |
|------|-----|
| 编程语言 | C++17 / Qt 6 Widgets |
| 项目名称 | codexx_v15_ai_butler |
| 总预估 | 9.5 天（4 个子版本） |
| 现有测试基线 | 57/57 全部通过 |

---

## 2. 产品定义

### 2.1 Product Goals

1. **可编程的自动调度**：Agent 能按 Cron 表达式定时执行任务，不再需要用户手动触发
2. **人格化的角色系统**：支持多角色、情感状态、叙事上下文，让 Agent 变成"有个性的管家"
3. **可量化的使用统计**：用户能直观看到 Agent 的使用数据、历史决策和聊天记录全搜
4. **开放的工具生态**：通过 MCP 协议对接外部工具，不再局限于内置 39 个工具

### 2.2 当前 vs 目标

| 能力 | 当前 (V14) | V15 后 |
|------|-----------|--------|
| 任务触发 | 用户手动输入每次 | 定时 Cron + 事件驱动 |
| 角色 | 单一 system prompt | 可切换多角色 + 情感状态 |
| 对话连续性 | 三层记忆 | 三层记忆 + 叙事上下文 |
| 聊天记录 | 基本存储 | 全文搜索 + 统计面板 |
| 工具生态 | 39 个内置工具 | 内置 + MCP 外部工具 |

---

## 3. 子版本详细设计

---

### V15.1：调度层 — Cron 定时器 + 任务队列（2 天）

#### 目标
让用户能设置"每天 9 点帮我检查邮件""每周五下午 5 点生成周报"等定时任务，无需手动触发。

#### 核心数据结构

```cpp
// src/scheduler/ScheduledTask.h（新增）
struct ScheduledTask {
    QString id;                  // UUID
    QString name;                // "每日邮件检查"
    QString cronExpression;      // "0 9 * * *"
    QString agentPrompt;         // 传给 Agent 的指令
    bool enabled = true;
    QDateTime lastRun;
    QDateTime nextRun;
    int maxRetries = 3;
};

// src/scheduler/TaskScheduler.h（新增）
class TaskScheduler : public QObject {
    Q_OBJECT
public:
    void addTask(const ScheduledTask &task);
    void removeTask(const QString &taskId);
    QVector<ScheduledTask> allTasks() const;
    void start();  // 启动调度循环
    void stop();

signals:
    void taskTriggered(const ScheduledTask &task);

private:
    QTimer *m_timer;            // 每分钟检查一次
    QVector<ScheduledTask> m_tasks;
    // Cron 解析器：* * * * * → QDateTime
    QDateTime nextRunTime(const QString &cron, const QDateTime &from);
};
```

#### Cron 解析器

```cpp
// 支持标准 5 字段 Cron：分钟 小时 日 月 星期
// 示例：0 9 * * *      → 每天 9:00
//       30 17 * * 5     → 每周五 17:30
//       0 */2 * * *     → 每 2 小时

// 解析策略：手写轻量解析器，不引入第三方库
// 5 字段逐个解析，支持 * 、具体值、/step 三种语法
```

#### 任务存储

```
~/.codex/scheduled_tasks.json  ← 用户级
.workbuddy/scheduled_tasks.json ← 项目级
```

#### 执行流程

```
TaskScheduler::m_timer → 每分钟 tick()
  → 遍历所有 enabled 任务
  → 如果 now >= task.nextRun：
      → emit taskTriggered(task)
      → ApplicationController 收到信号
      → 自动调用 sendAgentLoopMessage(task.agentPrompt)
      → Agent 循环执行
      → 完成后：更新 task.lastRun, task.nextRun
      → 写入执行日志到 L3 记忆
```

#### UI 设计

设置窗口 → 新增「调度」标签页：

```
┌──────────────────────────────────────────────┐
│ 设置                                          │
│ [API] [模型] [语言] [技能] [Hook] [调度]      │
├──────────────────────────────────────────────┤
│ 定时任务 (2)                   [+ 添加任务]   │
│ ┌──────────────────────────────────────────┐  │
│ │ ✅ 每日代码审查    每天 09:00   下次: 明天  │  │
│ │    prompt: 审查今日提交的代码变更         │  │
│ │                                            │  │
│ │ ✅ 周报生成        每周五 17:30  下次: 后天 │  │
│ │    prompt: 生成本周工作总结               │  │
│ └──────────────────────────────────────────┘  │
└──────────────────────────────────────────────┘
```

#### 改动清单

| 文件 | 改动 |
|------|------|
| `src/scheduler/ScheduledTask.h` | 新增：任务数据结构 |
| `src/scheduler/TaskScheduler.h/.cpp` | 新增：Cron 解析 + 定时触发 |
| `src/scheduler/TaskStorage.h/.cpp` | 新增：JSON 持久化 |
| `src/app/ApplicationController.h/.cpp` | 新增 TaskScheduler 成员；连接 taskTriggered |
| `src/ui/SettingsDialog.h/.cpp` | 新增「调度」标签页（P1） |
| `src/ui/SchedulerWidget.h/.cpp` | 新增：任务列表 UI（P1） |
| `CMakeLists.txt` | 新增 scheduler 源文件 |

#### 测试

```
tests/scheduler/TaskSchedulerTest.cpp（新增，≥6 个）
tests/scheduler/CronParserTest.cpp（新增，≥6 个）
```

| 用例 | 说明 |
|------|------|
| cron-parse-daily | `0 9 * * *` → 正确解析 |
| cron-parse-weekly | `30 17 * * 5` → 正确解析 |
| cron-parse-hourly | `0 */2 * * *` → 正确解析 |
| cron-invalid | 无效 cron → 返回错误 |
| task-trigger | 到达时间后触发信号 |
| task-disabled | 禁用任务不触发 |
| task-persist | 持久化到 JSON → 恢复 |
| task-remove | 删除任务 → 文件同步 |

---

### V15.2：角色系统 — 多角色 + 情感状态 + 叙事上下文（3 天）

#### 目标
让 Agent 能扮演不同角色（如"暴躁码农""贴心管家""毒舌评论员"），每个角色有独立的情感状态和叙事上下文。

#### 角色定义格式

```json
// ~/.codex/roles/grumpy_coder.json
{
  "name": "grumpy_coder",
  "displayName": "暴躁码农",
  "description": "一个脾气暴躁但技术过硬的程序员",
  "systemPrompt": "你是一个暴躁的程序员。你对代码质量要求很高，看到烂代码会直接开喷...",
  "traits": {
    "patience": 3,
    "humor": 7,
    "strictness": 9,
    "creativity": 6
  },
  "moodStates": [
    {"name": "平静", "threshold": 0, "promptModifier": "语气平和..."},
    {"name": "不耐烦", "threshold": 50, "promptModifier": "开始烦躁..."},
    {"name": "暴怒", "threshold": 80, "promptModifier": "彻底爆发..."}
  ],
  "narrationStyle": "简洁的第三人称旁白，偶尔插入内心戏"
}
```

#### 角色管理器

```cpp
// src/persona/RoleDefinition.h（新增）
struct RoleDefinition {
    QString name;
    QString displayName;
    QString description;
    QString systemPrompt;
    QMap<QString, int> traits;
    QVector<MoodState> moodStates;
    QString narrationStyle;
};

// src/persona/MoodState.h（新增）
struct MoodState {
    QString name;
    int threshold;        // mood 值超过此阈值则切换到此状态
    QString promptModifier; // 追加到 system prompt 的修饰语
};

// src/persona/RoleManager.h（新增）
class RoleManager {
public:
    void loadFromDirectory(const QString &dir);
    const RoleDefinition *currentRole() const;
    void switchRole(const QString &name);
    QStringList availableRoles() const;

    // 情感系统
    int currentMood() const;
    void adjustMood(int delta);
    QString moodPromptModifier() const;
};
```

#### 叙事上下文

在 Agent 循环每次迭代后，追加一段简短的叙事文本到会话上下文：

```
[叙事] 暴躁码农皱起眉头，盯着屏幕上的错误日志。第 3 次构建失败了。他开始有点不耐烦了。
```

叙事风格由角色定义，AI 在每轮循环后根据当前情感状态生成叙事。

#### UI 设计

设置窗口 → 新增「角色」标签页 + 主界面状态栏：

```
主界面侧边栏：
┌──────────────┐
│ 💬 聊天      │
│ 🤖 Agent     │
│ 📅 调度      │
│ 👤 管家      │  ← 当前：暴躁码农 🤬 mood: 72
│   [切换角色]  │
└──────────────┘
```

#### 改动清单

| 文件 | 改动 |
|------|------|
| `src/persona/RoleDefinition.h` | 新增：角色数据结构 |
| `src/persona/RoleManager.h/.cpp` | 新增：角色加载/切换/情感 |
| `src/persona/NarrationGenerator.h/.cpp` | 新增：叙事文本生成（调用 LLM） |
| `src/app/ApplicationController.cpp` | 注入角色 systemPrompt + mood modifier |
| `src/app/AgentLoopPromptBuilder.cpp` | 在 prompt 中追加叙事 |
| `src/ui/MainWindow.h/.cpp` | 侧边栏角色区 |
| `CMakeLists.txt` | 新增 persona 源文件 |

#### 测试

```
tests/persona/RoleManagerTest.cpp（新增，≥6 个）
```

---

### V15.3：使用统计 + 全文搜索（1.5 天）

#### 统计面板

```
┌──────────────────────────────────────────────┐
│ 使用统计                    本周 | 本月 | 全部 │
├──────────────────────────────────────────────┤
│                                              │
│  📊 总对话次数        247                    │
│  🤖 Agent 任务完成率   89% (142/160)         │
│  🛠️ 工具调用总数        1,203                │
│  ⏱️ 平均任务耗时        42 秒                │
│  🔄 最常用工具          system.path          │
│  📁 文件操作            387                  │
│                                              │
│  [最近 7 天用量趋势图]                       │
│  ████▌   ████▌   ███▌    ███     ██▌   █    │
│   周一    周二    周三    周四   周五  周六 周日│
│                                              │
│  [最活跃会话 Top 5]                           │
│  1. Agent 循环测试    135 轮                 │
│  2. 代码审查助手      89 轮                  │
└──────────────────────────────────────────────┘
```

#### 全文搜索

设置窗口 → 新增「搜索」标签页：

```
┌──────────────────────────────────────────────┐
│ 搜索聊天记录                    [🔍_________] │
├──────────────────────────────────────────────┤
│ 找到 12 条结果，关键词「构建」                 │
│ ┌──────────────────────────────────────────┐  │
│ │ 6/4 14:32  Agent: cmake --build 失败     │  │
│ │             原因：缺少 Qt6::Sql 链接      │  │
│ │                                           │  │
│ │ 6/3 21:15  Agent: 构建成功，3.7MB        │  │
│ │             52/52 测试通过               │  │
│ │                                           │  │
│ │ 6/3 20:43  User:  帮我重新构建项目       │  │
│ └──────────────────────────────────────────┘  │
└──────────────────────────────────────────────┘
```

#### 数据来源

统计数据从现有基础设施自动收集：
- L3 每日日志（对话数、Agent 任务数、工具调用数）
- ChatHistoryStorage（已有 SQLite 存储）
- 统计计算在 UI 打开时即时计算，无需额外后台采集

#### 改动清单

| 文件 | 改动 |
|------|------|
| `src/stats/UsageStatistics.h/.cpp` | 新增：统计数据计算 |
| `src/stats/ChatSearchEngine.h/.cpp` | 新增：全文搜索 |
| `src/ui/StatsWidget.h/.cpp` | 新增：统计面板 UI |
| `src/ui/SearchWidget.h/.cpp` | 新增：搜索界面 UI |
| `CMakeLists.txt` | 新增 stats 源文件 |

#### 测试

```
tests/stats/UsageStatisticsTest.cpp（新增，≥4 个）
tests/stats/ChatSearchEngineTest.cpp（新增，≥4 个）
```

---

### V15.4：MCP 协议支持（3 天）

#### 目标

让 CodeXX 的工具注册表能对接外部 MCP (Model Context Protocol) 服务器，实现工具生态互通。

#### MCP 协议简版

```
Client (CodeXX)                    Server (任意语言)
     │                                  │
     ├─ tools/list ────────────────────→│  ← 获取工具列表
     │←───────────────── [tool1, tool2] │
     │                                  │
     ├─ tools/call ────────────────────→│  ← 调用工具
     │   {name: "search", args: {...}}   │
     │←────────────── {result: "..."}   │
     │                                  │
     ├─ resources/list ────────────────→│  ← 获取资源列表
     │←─────────── [resource1, ...]     │
```

通信方式：JSON-RPC over stdio（QProcess 连接 MCP 服务器进程）

#### 核心设计

```cpp
// src/mcp/McpConnector.h（新增）
class McpConnector : public QObject {
    Q_OBJECT
public:
    // 启动 MCP 服务器进程（如 npx @modelcontextprotocol/server-xxx）
    bool connectToServer(const QString &command, const QStringList &args);
    void disconnect();

    // 获取服务器提供的工具定义
    QVector<McpToolDefinition> listTools();

    // 调用 MCP 工具
    ToolResult callTool(const QString &name, const QJsonObject &args);

    // 获取资源列表
    QVector<McpResource> listResources();

private:
    QProcess *m_process;
    int m_nextRequestId = 1;
    // JSON-RPC 消息编解码
    QJsonObject sendRequest(const QString &method, const QJsonObject &params);
};
```

#### MCP 配置格式

```json
// ~/.codex/mcp_servers.json
{
  "servers": [
    {
      "name": "filesystem",
      "command": "npx",
      "args": ["-y", "@modelcontextprotocol/server-filesystem", "/path/to/allowed/dir"],
      "enabled": true
    },
    {
      "name": "sqlite",
      "command": "uvx",
      "args": ["mcp-server-sqlite", "--db-path", "/path/to/db.sqlite"],
      "enabled": true
    }
  ]
}
```

#### 工具注册表扩展

```cpp
// AgentToolRegistry 新增方法
void AgentToolRegistry::registerExternalTools(
    const QVector<McpToolDefinition> &mcpTools,
    McpConnector *connector);

// 执行时判断来源：
// - 内置工具 → 直接调用 execute()
// - MCP 工具 → 通过 McpConnector::callTool() 转发
```

#### 改动清单

| 文件 | 改动 |
|------|------|
| `src/mcp/McpConnector.h/.cpp` | 新增：JSON-RPC 通信 |
| `src/mcp/McpRegistry.h/.cpp` | 新增：MCP 服务器管理 |
| `src/tools/AgentToolRegistry.h/.cpp` | 修改：支持外部工具注册 |
| `src/app/ApplicationController.cpp` | 初始化 MCP 连接 |
| `CMakeLists.txt` | 新增 mcp 源文件 |

#### 测试

```
tests/mcp/McpConnectorTest.cpp（新增，≥6 个）
```

---

## 4. 需求优先级总览

| 优先级 | 子版本 | 内容 | 理由 |
|:---:|--------|------|------|
| **P0** | V15.1 | Cron 定时器 + 任务队列 | "个人管家"最基本能力：不用手动触发 |
| **P0** | V15.4 | MCP 协议支持 | 工具体系开环，最影响能力天花板 |
| **P1** | V15.3 | 使用统计 + 全文搜索 | QoL 需求，方便用户了解 AI 使用情况 |
| **P2** | V15.2 | 角色系统 | 酷炫但非核心，取决于用户是否真需要多角色 |

**建议开发顺序**：V15.1（调度）→ V15.4（MCP）→ V15.3（统计/搜索）→ V15.2（角色）

因为 V15.1 和 V15.4 都扩展了 Agent 的核心执行能力，V15.3 是体验增强，V15.2 是最"花哨"的。

---

## 5. 依赖关系

```
V15.1 调度层 ─────────┐
                       ├─→ V15.3 统计/搜索（统计面板需要调度执行数据）
V15.4 MCP 协议 ───────┘
                       │
V15.2 角色系统 ───────（独立，与以上三者无强依赖）
```

V15.1 和 V15.4 可并行开发；V15.2 完全独立。

---

## 6. 验收标准

| AC | 内容 |
|----|------|
| AC-1 | Cron 定时器正确解析并按时触发 Agent 任务 |
| AC-2 | 定时任务持久化到 JSON，重启不丢失 |
| AC-3 | MCP 服务器连接成功，工具列表正确同步 |
| AC-4 | MCP 工具调用结果正确返回 |
| AC-5 | 统计面板正确展示对话/Agent/工具数据 |
| AC-6 | 全文搜索在聊天记录中找到匹配结果 |
| AC-7 | 角色切换后 system prompt 和 mood 正确变更 |
| AC-8 | `ctest` 新增 ≥28 个测试，全量无回归 |
