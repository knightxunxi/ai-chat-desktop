# CodeXX 运维学习 — 结合项目实战与岗位知识

> 以 CodeXX 真实代码为主线，每章先看源码再学原理

---

## 一、运维三板斧总览

| 运维领域 | CodeXX 对应模块 | 学了能做什么 |
|---------|---------------|-----------|
| **日志管理** | `AppLogger` + `LogFileReader` | 排查线上故障、审计 Agent 行为 |
| **配置管理** | `ConfigStorage` + `AppConfig` | 多环境切换、API Key 安全管理 |
| **持久化与备份** | `ChatHistoryStorage` + `AgentLoopState` + `TaskStorage` | 数据恢复、崩溃重连 |
| **安全与防护** | `BuiltinHooks` + `CommandPolicy` + `WindowsCredentialStorage` | 注入防护、凭证管理 |
| **错误恢复** | `handleRequestFailed` 三级回退 | 自愈系统设计 |
| **进程管理** | `QProcess` (MCP, OCR, Command) | 外部进程生命周期 |
| **性能监控** | `TokenBar` + `RateLimitHook` | 资源使用追踪、限速保护 |

---

## 二、日志管理

### 2.1 AppLogger — 日志就是运维的眼睛

```
设计问题: 为什么不用 qDebug() 而自己写 AppLogger？
答案: qDebug() 只在 Debug 模式输出，Release 无日志。
    AppLogger 写入文件，Release 生产环境也能查。
```

#### 核心代码（`src/support/AppLogger.cpp`）

| 功能 | 方法 | 说明 |
|------|------|------|
| 初始化 | `init(path)` | 指定日志文件路径，默认 `~/.codex/app.log` |
| 信息日志 | `info(tag, msg)` | 普通操作记录 |
| 警告日志 | `warning(tag, msg)` | 非致命异常 |
| 错误日志 | `error(tag, msg)` | 致命错误 |
| 日志格式化 | 内部 | `[2026-06-21 17:00:00] [Info] [AgentLoop] Step 3 completed` |

#### LogFileReader — 日志怎么查

```cpp
// 运维场景: 用户反馈 "Agent 昨晚报错了"
// → 打开 LogViewerDialog，搜索 "Error"
LogFileReader reader("~/.codex/app.log");
auto lines = reader.readLast(1000); // 读最后 1000 行
```

#### 知识点延伸

| 概念 | 学什么 |
|------|--------|
| **日志级别** | DEBUG < INFO < WARN < ERROR < FATAL |
| **日志轮转** | 单文件增长到 10MB 自动归档（本项目未实现但可以加） |
| **结构化日志** | JSON 格式便于 ELK 采集（本项目是文本格式） |
| **敏感脱敏** | 日志中不能出现 API Key、密码 → `SensitiveFilterHook` |

#### 要看的文件

- `src/support/AppLogger.h` + `.cpp`
- `src/support/LogFileReader.h` + `.cpp`
- `src/ui/LogViewerDialog.h` + `.cpp`（内置日志查看器）

---

## 三、配置管理

### 3.1 AppConfig — 所有设置的中心

```cpp
// src/core/AppConfig.h
struct AppConfig {
    QString providerName;        // "DeepSeek" | "OpenAI" | "Custom"
    QString baseUrl;             // "https://api.deepseek.com/v1"
    QString modelName;           // "deepseek-v4-flash"
    QString apiKey;              // 加密存储
    double temperature;          // 0.0 - 2.0
    int maxTokens;               // 最大输出长度
    AppLanguage language;        // 中/英
    QString agentWorkspaceDir;   // Agent 工作目录
    QString agentProjectDir;     // 项目目录
};
```

#### ConfigStorage — 持久化到 SQLite

```cpp
// 保存逻辑: AppConfig → JSON → SQLite 数据库
ConfigStorage storage;
storage.save(config);

// 读取逻辑: SQLite → JSON → AppConfig
auto config = storage.load();
```

#### 知识点延伸

| 概念 | 学什么 |
|------|--------|
| **配置分层** | 默认值 → 用户配置 → 环境变量 → 命令行参数（优先级递增） |
| **热重载** | 配置变更后无需重启应用（本项目用信号通知，但非热重载） |
| **敏感配置** | API Key 不应明文存储 → `WindowsCredentialStorage` |

#### 要看的文件

- `src/core/AppConfig.h`
- `src/storage/ConfigStorage.h` + `.cpp`
- `src/storage/WindowsCredentialStorage.h` + `.cpp`

---

## 四、持久化与数据恢复

### 4.1 会话历史 → ChatHistoryStorage

```
每次聊天完毕 → ChatHistoryStorage::save(session)
                  ↓
            SQLite: sessions 表 + messages 表
                  ↓
            ChatView 重新渲染: populateChatView()
```

#### SQLite 表结构（推测，从代码推断）
```sql
CREATE TABLE sessions (
    id TEXT PRIMARY KEY,
    title TEXT,
    system_prompt TEXT,
    created_at TEXT,
    updated_at TEXT,
    is_favorite INTEGER DEFAULT 0,
    is_archived INTEGER DEFAULT 0
);

CREATE TABLE messages (
    id TEXT PRIMARY KEY,
    session_id TEXT REFERENCES sessions(id),
    role TEXT,           -- 'user' | 'assistant' | 'system'
    content TEXT,
    created_at TEXT
);
```

### 4.2 Agent 崩溃恢复 → AgentLoopState

```
Agent 循环中每完成一步 →
    AgentLoopState { goal, stepIndex, accumulatedResults }
        → toJson() → 写入 ~/.codex/agent_state.json

应用重启 → 检测 agent_state.json 存在 →
    弹出 "上次任务在第 N 步中断，是否继续？"
        → 是：注入已完成步骤 → 继续循环
        → 否：删除状态文件
```

#### 这是运维最核心的"自愈"能力

| 概念 | 学什么 |
|------|--------|
| **检查点机制** | 每 N 步保存快照，崩溃后从最近检查点恢复 |
| **幂等性** | 恢复后重复执行已完成的步骤应该无副作用 |
| **状态文件清理** | 任务完成/取消后立即删除，避免启动误判 |

### 4.3 任务持久化 → TaskStorage

```
用户在 UI 创建定时任务 →
    ScheduledTask → TaskStorage::save(allTasks())
        → JSON 写入 .workbuddy/scheduled_tasks.json

应用重启 → TaskStorage::load() → TaskScheduler::addTask()
    → 恢复所有定时任务，继续按 cron 触发
```

### 4.4 三层记忆 → ProjectMemoryManager

```
L1: ~/.codex/MEMORY.md         ← 跨项目用户偏好
L2: .workbuddy/memory/MEMORY.md ← 项目技术决策
L3: .workbuddy/memory/YYYY-MM-DD.md ← 每日日志（只追加）

超过 14 天的旧日志 → LLM 生成压缩摘要 → YYYY-Www-compressed.md
```

#### 知识点延伸

| 概念 | 学什么 |
|------|--------|
| **数据目录规范** | XDG/LocalAppData 标准路径 |
| **JSON vs SQLite** | JSON 适合少量配置；SQLite 适合大量查询 |
| **备份策略** | 热备份 SQLite (`.backup`)、定时 rsync JSON |

#### 要看的文件

- `src/storage/ChatHistoryStorage.h` + `.cpp`
- `src/core/AgentLoopState.h`
- `src/scheduler/TaskStorage.h` + `.cpp`
- `src/memory/ProjectMemoryManager.h` + `.cpp`
- `src/memory/DailyMemoryWriter.h` + `.cpp`

---

## 五、安全与防护

### 5.1 SensitiveFilterHook — 凭证泄露防护

```cpp
// 问题: AI 的回复里会不会意外包含用户的 API Key？
// 答案: PostReceive 阶段自动过滤

输入: "这是你的 API Key: sk-or-v1-abc123def456..."
      ↓ SensitiveFilterHook::execute()
      ↓ QRegularExpression 匹配 sk-.* / ghp_.* / AIza.* / Bearer .*
输出: "这是你的 API Key: [REDACTED]"
```

#### 支持的正则模式

| 模式 | 匹配内容 | 例子 |
|------|---------|------|
| `sk-[a-zA-Z0-9]{20,}` | OpenAI Key | sk-or-v1-abc... |
| `ghp_[a-zA-Z0-9]{20,}` | GitHub Token | ghp_x1x2x3x4... |
| `AIza[0-9A-Za-z\-_]{20,}` | Google API Key | AIzaSyA1b2... |
| `Authorization:\s*Bearer\s+...` | HTTP 认证头 | Authorization: Bearer eyJhb... |

#### 知识点延伸

| 概念 | 学什么 |
|------|--------|
| **凭证轮换** | API Key 应定期更换 |
| **最小权限** | Agent 工作目录外的文件不可读写 |
| **环境变量注入** | API Key 不应硬编码在代码或配置文件中 |
| **日志脱敏** | 日志中不能包含密码、token、手机号 |

### 5.2 RateLimitHook — API 滥用防护

```cpp
// 同一 sessionId 60 秒内最多 20 次请求

void RateLimitHook::execute(ctx) {
    // 清理 60 秒前的时间戳
    timestamps.erase(timestamps.begin(), cutoff);
    
    if (timestamps.size() >= 20) {
        return Reject("速率限制：每分钟最多 20 次");
    }
    
    timestamps.append(now);
    return Pass;
}
```

### 5.3 CommandPolicy — 命令执行安全

```cpp
// Agent 不能执行任意命令，只能通过白名单

CommandPolicy::isAllowed(command):
    allowed = ["cmake --build", "git status", "dir", "ls", ...]
    
    if command not in allowed:
        return false; // 拒绝执行
```

### 5.4 WindowsCredentialStorage — 密钥安全存储

```cpp
// API Key 不存明文，存到 Windows 凭据管理器

WindowsCredentialStorage::store("codexx-api-key", apiKey);
// → Windows Credential Manager (Control Panel → Credential Manager)
// → 加密存储，只有当前用户能访问
```

#### 要看的文件

- `src/hooks/BuiltinHooks.h` + `.cpp`
- `src/tools/core/CommandPolicy.h` + `.cpp`
- `src/storage/WindowsCredentialStorage.h` + `.cpp`

---

## 六、错误恢复 — 三级自愈系统

### 6.1 上下文溢出 → 三级回退

```
handleRequestFailed("context_length exceeded")
    │
    ├─ 第1级: compressObservations()
    │   保留最近 50% 的 observations，其余标记为 [context-compressed]
    │   → 重试发送 (最多 3 次)
    │
    ├─ 第2级: truncateCurrentSession()
    │   硬截断对话历史到安全窗口
    │   → 重试发送 (最多 2 次)
    │
    └─ 第3级: fallbackToMinimalContext()
        只保留 system prompt + 最后一条用户消息
        → 重试发送
                 ↓
        仍失败: 展示错误 + 启用重试按钮
```

### 6.2 代码对应

```cpp
// ApplicationController.cpp:1404-1430

void handleRequestFailed(message, category) {
    if (isContextOverflow && m_retryAfterCompressionCount < 3) {
        // 第 1 级: 压缩
        m_retryAfterCompressionCount++;
        compressObservations();
        sendAgain();
    } else if (需要截断) {
        // 第 2 级: 截断
        truncateSession();
        sendAgain();
    } else {
        // 第 3 级: 最小上下文
        fallbackToMinimalContext();
        sendAgain();
    }
}
```

#### 知识点延伸

| 概念 | 学什么 |
|------|--------|
| **断路器模式** | 连续失败 N 次 → 停止调用 → 等待恢复 |
| **重试策略** | 指数退避（等待 1s、2s、4s、8s 递增） |
| **优雅降级** | 部分功能不可用时，提供最小可用版本 |

#### 要看的文件

- `src/app/ApplicationController.cpp` 搜索 `handleRequestFailed`
- `src/app/ApplicationController.cpp` 搜索 `compress` 和 `truncate`
- `src/app/ContextWindowManager.h` + `.cpp`

---

## 七、进程管理

### 7.1 QProcess 在项目中的使用

| 模块 | 用途 | QProcess 生命周期 |
|------|------|:---:|
| `McpConnector` | 连接外部 MCP 服务器 | 长连接，JSON-RPC 交互 |
| `ScreenCaptureService` | 截图工具 | 短连接，执行完退出 |
| `OcrService` | OCR 识别 (PowerShell) | 短连接，30s 超时 |
| `CommandRunner` | Agent 执行命令 | 短连接，结果捕获 |
| `ScriptHookRunner` | 外部 Hook 脚本 | 白名单 + 10s 超时 |

### 7.2 典型 QProcess 管理模式

```cpp
// 短连接模式
QProcess process;
process.start("cmd", {"/c", "dir"});
process.waitForFinished(10000); // 10秒超时
QString output = process.readAllStandardOutput();

// 长连接模式 (MCP)
QProcess *mcpProcess = new QProcess();
mcpProcess->start("npx", {"-y", "@modelcontextprotocol/server-filesystem"});
connect(mcpProcess, &QProcess::readyRead, this, [this]() {
    handleJsonRpcResponse(mcpProcess->readAll());
});
```

#### 要看的文件

- `src/mcp/McpConnector.h` + `.cpp`
- `src/tools/core/CommandRunner.h` + `.cpp`
- `src/hooks/ScriptHookRunner.h` + `.cpp`

---

## 八、性能监控

### 8.1 TokenBar — 资源使用可视化

```cpp
// 每次消息更新后估算 token 数
int used = estimateTokenCount(allMessages);

// TokenBar: 绿色(<80%) → 橙色(80-95%) → 红色(>95%)
m_tokenBar->updateTokens(used, 128000); // 128K 窗口
```

### 8.2 其他监控指标

| 指标 | 怎么获取 | 作用 |
|------|---------|------|
| API 调用次数 | `m_agentLoopIteration` 计数器 | 成本预估 |
| 工具执行耗时 | `QElapsedTimer` 每步计时 | 性能瓶颈定位 |
| 内存占��� | Windows: `GetProcessMemoryInfo` | 内存泄漏检测 |
| 日志文件大小 | `QFileInfo(LogPath).size()` | 磁盘空间管理 |

#### 要看的文件

- `src/ui/TokenBar.h` + `.cpp`
- 搜索 `QElapsedTimer` 查看所有计时点

---

## 九、构建与部署

### 9.1 CMake 多构建模式

```
build/              ← Debug 构建 (CMAKE_BUILD_TYPE=Debug)
build-qt/           ← Debug 构建 (历史)
build-release/      ← Release 构建 (CMAKE_BUILD_TYPE=Release)
build-release-qt/   ← Release 构建 (历史)
release/            ← 打包输出：exe + 28 dll + 32 qm
```

### 9.2 MinGW 编译特殊处理

```cmake
# CMakeLists.txt:199-206
if(MINGW)
    target_link_options(AIChatDesktop PRIVATE -mwindows)
    set_target_properties(AIChatDesktop PROPERTIES WIN32_EXECUTABLE FALSE)
endif()
```

MinGW 特有问题的解决：`WIN32_EXECUTABLE` 导致 `__imp___argc` 链接错误 → 用 `-mwindows` 手动抑制控制台窗口。

### 9.3 Release 打包清单

```
AIChatDesktop.exe          ← 主程序
Qt6Core.dll                ← Qt 核心库
Qt6Widgets.dll             ← Qt UI 库
Qt6Network.dll             ← Qt 网络库
Qt6Sql.dll                 ← Qt 数据库库
... (28 个 DLL)
translations/*.qm          ← 32 个语言翻译文件
```

#### 知识点延伸

| 概念 | 学什么 |
|------|--------|
| **windeployqt** | Qt 提供的自动收集依赖 DLL 工具 |
| **Inno Setup / NSIS** | Windows 安装包制作 |
| **GitHub Releases** | 自动发布 + 版本号管理 |
| **CI/CD** | 每次 push 自动编译 + 打包 + 发布 |

#### 要看的文件

- `CMakeLists.txt`
- `release/` 目录结构

---

## 十、运维学习路线

### 第 1 天：日志 + 配置
```
读: AppLogger.cpp → ConfigStorage.cpp → WindowsCredentialStorage.cpp
练: 在 LogViewerDialog 中按关键字搜索日志
问: 如果日志文件超过 100MB 怎么办？
```

### 第 2 天：持久化 + 恢复
```
读: ChatHistoryStorage.cpp → AgentLoopState.h → TaskStorage.cpp
练: 模拟崩溃恢复：Agent 执行到第 3 步 → 关闭程序 → 重启 → 验证恢复
问: 如果 agent_state.json 损坏了怎么办？
```

### 第 3 天：安全 + 错误恢复
```
读: BuiltinHooks.cpp → CommandPolicy.cpp → ApplicationController::handleRequestFailed
练: 制造一个 context_length_exceeded 错误 → 观察三级回退流程
问: 如果 API Key 在日志中泄露了，SensitiveFilterHook 能拦住吗？
```

### 第 4 天：进程管理 + 打包发布
```
读: McpConnector.cpp → ScriptHookRunner.cpp → release/
练: 在 Release 模式下编译并打包 → 在另一台电脑运行
问: 打包后缺少 DLL 怎么自动检测？(windeployqt)
```

---

## 附录：源码精确行号参考

> 以下来自对运维相关代码的逐文件探索，标注了实际行号，方便精确跳转阅读

### 日志系统

| 内容 | 文件 | 行号 |
|------|------|:---:|
| 日志路径: `QStandardPaths::AppDataLocation/ai-chat-desktop.log` | AppLogger.cpp | 39-44 |
| 日志脱敏: Bearer token 和 api_key=value 自动替换 | AppLogger.cpp | 26-35 |
| 线程安全: 全局 QMutex | AppLogger.cpp | 14-18 |
| 格式: `ISO时间 [级别] [类别] 内容` | AppLogger.cpp | 87-91 |
| 读取最后N行 (无文件大小限制 ⚠️) | LogFileReader.cpp | 7 |

### Agent 状态持久化

| 内容 | 文件 | 行号 |
|------|------|:---:|
| AgentLoopState 结构体定义 | AgentOrchestrator.h | 147 |
| 持久化路径 | AgentOrchestrator.cpp | 719-723 |
| 初始化静默检测残留状态 | AgentOrchestrator.cpp | 102 |
| startAgentLoop 保存初始快照 | AgentOrchestrator.cpp | 138 |
| 每轮迭代后保存 | AgentOrchestrator.cpp | 301 |
| 清除状态文件 | AgentOrchestrator.cpp | 183 |

### 任务 JSON 持久化

| 内容 | 文件 | 行号 |
|------|------|:---:|
| 保存前 mkdir | TaskStorage.cpp | 13 |
| 加载静默容错 | TaskStorage.cpp | 14 |

### 记忆系统

| 内容 | 文件 | 行号 |
|------|------|:---:|
| L1 路径 | ProjectMemoryManager.cpp | 251 |
| L2 路径 | ProjectMemoryManager.cpp | 256 |
| 敏感检测关键词 (api_key/token/password/secret/bearer/credential/private_key) | ProjectMemoryManager.cpp | 35-63 |
| 记忆预算: 30K 总上限, 2K/条, 50条上限, 14天窗口 | ProjectMemoryManager.h | 多处 |
| 压缩: ISO周分组→LLM提示词 | ProjectMemoryManager.cpp | 309 |
| 压缩落盘: YYYY-Www-compressed.md | ProjectMemoryManager.cpp | 392 |

### 安全防护

| 内容 | 文件 | 行号 |
|------|------|:---:|
| TimestampHook (PreSend 注入 UTC) | BuiltinHooks.cpp | 11-26 |
| RateLimitHook (60秒20次, 按 sessionId 分桶) | BuiltinHooks.cpp | 32-66 |
| SensitiveFilterHook (5种正则: sk-/ghp_/AIza/Bearer/Authorization) | BuiltinHooks.cpp | 72-132 |
| ScriptHookRunner 路径白名单 (~/.codex/hooks/, .workbuddy/hooks/) | ScriptHookRunner.cpp | 34-45 |
| 环境变量白名单 (仅5个) | ScriptHookRunner.cpp | 58-67 |
| 脚本: QProcess 程序+参数数组, 不经过 shell | ScriptHookRunner.cpp | 98 |
| CommandPolicy 白名单命令 (6个) | CommandPolicy.cpp | 多处 |
| 黑名单程序 (13个: powershell/cmd/del/shutdown等) | CommandPolicy.cpp | 219-261 |
| 命令安全链: 模板ID→程序名检查→工作目录安全→Shell元字符→黑名单 | CommandPolicy.cpp | 219-261 |
| 命令脱敏输出 (redactSensitiveOutput + truncateOutput 4000字符) | CommandRunner.cpp | 10-42 |
| 命令执行: SeparateChannels + 3s启动超时 + kill+2s收尾 | CommandRunner.cpp | 120-161 |

### 错误恢复

| 内容 | 文件 | 行号 |
|------|------|:---:|
| kMaxRetryAfterCompression = 3 | ApplicationController.h | 212 |
| kMaxTruncationResume = 2 | ApplicationController.h | 214 |
| 第一层: Reactive压缩 | ApplicationController.cpp | 1407-1439 |
| 第二层: 原生工具降级 | ApplicationController.cpp | 1515-1537 |
| 第三层: 用户可见错误+重试按钮 | ApplicationController.cpp | 1491-1512 |
| 响应截断自动续接 (最多2次) | ApplicationController.cpp | 1055-1094 |

### 数据持久化

| 内容 | 文件 | 行号 |
|------|------|:---:|
| 聊天历史: SQLite (chat-history.db) | ChatHistoryStorage.cpp | — |
| sessions 表含 agent_steps_json + branches_json | ChatHistoryStorage.cpp | — |
| 渐进式迁移 (PRAGMA table_info + ALTER TABLE) | ChatHistoryStorage.cpp | — |
| 配置: QSettings("AIChatDesktop", "AIChatDesktop") | ConfigStorage.cpp | — |
| API Key: Windows Credential Manager (CRED_PERSIST_LOCAL_MACHINE) | WindowsCredentialStorage.cpp | 16-90 |
| 旧 apiKey 自动迁移到 CredentialStorage | ConfigStorage.cpp | 77-90 |

### 构建部署

| 内容 | 文件 | 行号 |
|------|------|:---:|
| 12 个静态库依赖链: core→support→storage→services→memory→skills→hooks→scheduler→mcp→tools→app→ui | CMakeLists.txt | — |
| MinGW: -mwindows + WIN32_EXECUTABLE FALSE | CMakeLists.txt | 199-206 |
| Windows: 链接 Advapi32 (凭据管理器 API) | CMakeLists.txt | — |

