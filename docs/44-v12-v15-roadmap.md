# V12—V15 开发规划：从工具中介到感知-操作闭环

**基于：** [优化方向.md](优化方向.md) 第九节深度对标分析（2026-06-03）

**前置条件：** V11 已验收，46 个测试全部通过，Chat/Agent 统一模式可用。

---

## 1. 重新校准：为什么旧 V12-V14 路线需要调整

### 1.1 旧路线的问题

旧的 `39-v11-plus-development-roadmap.md` 中的 V12-V14 规划（电脑感知 → 设备输入 → 管家整合）在方向上是正确的，但存在三个关键缺失：

| 缺失 | 影响 | 对标系统的做法 |
|------|------|---------------|
| **无上下文管理** | 消息多了直接 413 报错，用户无法继续对话 | Claude Code 六层压缩，WorkBuddy/OpenClaw 自动 compact |
| **Agentic Loop 有硬上限** | 只能循环 2 轮，无法完成复杂任务 | 所有对标系统都是 `while(未终止)` 的无限循环 |
| **无跨会话记忆** | 每次新会话从零开始，Agent 没有"经验" | WorkBuddy 三层记忆，OpenClaw 持久化上下文 |

### 1.2 新路线原则

> **先修基础设施（循环、上下文、记忆），再堆工具和能力。**

```
旧路线：V12 电脑感知 → V13 设备输入 → V14 管家整合
新路线：V12 基础设施 → V13 记忆与技能 → V14 感知-操作闭环 → V15 深度整合
```

---

## 2. 版本路线总览

```
V11 ✅ (已完成)
  │
  ├─ V12：Agent 基础设施升级（P0，5.5 天）
  │   ├─ V12.1 上下文窗口管理（1.5 天）
  │   ├─ V12.2 无限 Agentic Loop（2 天）
  │   └─ V12.3 流式工具执行（2 天）
  │
  ↓
  V13：记忆系统与技能增强（P0，4 天）
  │   ├─ V13.1 三层记忆系统（2 天）
  │   ├─ V13.2 Skills 增强 + Hook 系统（1.5 天）
  │   └─ V13.3 V12/V13 整合验收（0.5 天）
  │
  ↓
  V14：电脑感知 → 操作闭环 MVP（P1，6 天）
  │   ├─ V14.1 感知层接入 Agent Loop（2 天）
  │   ├─ V14.2 操作层接入 Agent Loop（2 天）
  │   ├─ V14.3 感知-操作闭环 Demo（1.5 天）
  │   └─ V14.4 V14 验收（0.5 天）
  │
  ↓
  V15：个人 AI 管家整合（P2，待评估）
  │   ├─ V15.1 调度层（定时任务）
  │   ├─ V15.2 角色系统
  │   ├─ V15.3 使用统计面板
  │   └─ V15.4 MCP 协议支持
```

### 2.1 优先级矩阵

| 版本 | 优先级 | 工作量 | 判断依据 |
|------|--------|--------|----------|
| **V12** | **P0** | 5.5 天 | 当前最大体验缺陷——消息多了直接报错 |
| **V13** | **P0** | 4 天 | 让 Agent 有"经验"，每次不用重来 |
| V14 | P1 | 6 天 | 需要 V12/V13 基础设施就绪 |
| V15 | P2 | 待评估 | 长期整合，按需推进 |

---

## 3. V12：Agent 基础设施升级

### 3.0 版本定位

> **一句话：让 Agent 能处理长对话、能无限循环、能边思考边执行。**

做完 V12 后，Codex 的 Agent 循环能力达到 Claude Code / WorkBuddy / OpenClaw 同级。

### 3.1 当前能力矩阵（V12 前）

| 能力 | 状态 | 问题 |
|------|:---:|------|
| Agentic Loop | ⚠️ 有限（2轮） | 硬编码 maxSteps，无法处理变长任务 |
| 上下文管理 | ❌ 无 | 历史消息无限追加，触发 API 413 错误 |
| 流式工具执行 | ❌ | 必须等完整响应才能解析 tool_calls |
| 并发工具执行 | ❌ | 全部串行 |

### 3.2 目标能力矩阵（V12 后）

| 能力 | 状态 | 对标 |
|------|:---:|------|
| Agentic Loop | ✅ 无限循环 | Claude Code `while(true)` |
| 上下文管理 | ✅ 自动裁剪+摘要 | Claude Code 六层中最基础两层 |
| 流式工具执行 | ✅ StreamingToolExecutor | Claude Code 并行执行 |
| 并发工具执行 | ❌（P2，暂不做） | Claude Code 只读并发 |

---

### V12.1：上下文窗口管理（1.5 天）

#### 范围

新增 `ContextWindowManager` 模块，在 API 请求前自动检测 token 用量，超出上限时智能裁剪历史消息并用 AI 摘要替代裁剪部分。

#### 新增文件

```
src/app/ContextWindowManager.h
src/app/ContextWindowManager.cpp
```

#### 核心接口

```cpp
class ContextWindowManager {
public:
    struct ContextBuildResult {
        QVector<ChatMessage> messages;     // 实际发送的消息列表
        int estimatedTokens;               // 预估 token 数
        int originalMessageCount;          // 原始消息数
        int trimmedMessageCount;           // 裁剪掉的消息数
        int originalTokenEstimate;         // 原始 token 估算
        QString summary;                   // 被裁剪内容的 AI 摘要
    };

    // 构建上下文窗口
    ContextBuildResult build(
        const ChatSession &session,
        int maxTokens,
        const QString &systemPrompt,
        OpenAIClient *aiClient = nullptr   // 用于生成摘要，nullptr 则不生成
    );

    // Token 估算工具
    static int estimateTokens(const QString &text);
    // 中文 1 字符 ≈ 1.5 tokens, 英文/代码 ≈ 1 token/word ≈ 4 chars
};
```

#### 裁剪算法

```
1. 计算 systemPrompt + systemRolePrompt + 全部历史消息的 token 估算
2. 如果 ≤ maxTokens * 0.85 → 不做任何裁剪
3. 如果 ≥ maxTokens * 0.85：
   a. 保留 systemPrompt 和最近 3 轮对话（用户+AI）
   b. 从最旧的消息开始裁剪
   c. 如果提供了 aiClient：调用 AI 生成一句裁剪部分摘要
   d. 摘要以 system 角色注入消息列表开头
4. 保留的 messages 必须满足：
   - 第一条消息是 user 角色
   - 最后一条消息是 user 角色（符合 OpenAI API 规范）
```

#### 集成点

| 文件 | 改动 | 说明 |
|------|------|------|
| `ApplicationController.cpp` | `sendMessage()` 中调用 | 在构建 API 请求前裁剪 |
| `ApplicationController.cpp` | `generateAgentPlan()` 中调用 | Agent 模式也受益 |
| `ApplicationController.cpp` | `sendUnifiedMessage()` 中调用 | 统一模式也受益 |

#### 测试

```
tests/app/ContextWindowManagerTest.cpp
```

| 测试用例 | 说明 |
|----------|------|
| `estimateTokens_chinese` | 中文 token 估算准确性 |
| `estimateTokens_english` | 英文 token 估算准确性 |
| `estimateTokens_code` | 代码 token 估算准确性 |
| `build_noTrimWhenUnderLimit` | token 未超标时不裁剪 |
| `build_trimsOldestMessages` | 超标时裁剪最旧消息 |
| `build_keepsSystemPromptAndRecent` | 保留系统提示和最近消息 |
| `build_respectsRoleOrder` | 确保首尾消息角色正确 |
| `build_emptyInput` | 空输入不崩溃 |

#### 验收标准

- ⬜ 长会话能正常发送，不再 413 报错
- ⬜ 裁剪后有摘要注入，用户知道上下文被压缩
- ⬜ `ctest` 新增 8 个测试通过
- ⬜ 不影响短会话（消息数 < 10）的体验

---

### V12.2：无限 Agentic Loop（2 天）

#### 范围

将 `AgentLoopController` 从「预生成全量计划 → for 循环执行」升级为「每步动态生成下一步 → while 循环」。

#### 核心改动

**改造前（V8 架构）：**

```cpp
// AgentLoopController::executePlan()
// 1. 预先生成 5 步计划
// 2. for (step : plan.steps) { execute(step); }
// 3. 终止：步数耗尽 | 失败 | 停止
```

**改造后（V12 架构）：**

```cpp
// AgentLoopController::executeLoop()
// while (!shouldTerminate()) {
//     1. 构建本轮 Prompt（目标 + 上步结果 + 可用工具）
//     2. 调用 AI → 返回单步 action JSON
//     3. 解析 action（toolId + parameters）
//     4. 本地策略校验
//     5. 执行工具 → 收集 ToolResult
//     6. ToolResult 保存到 auditTrail
//     7. 检测终止条件
// }
//
// 终止条件: AI返回done=true | maxSteps(默认10) | 用户停止 | 超时
```

#### 修改文件

| 文件 | 改动内容 | 风险 |
|------|----------|------|
| `AgentLoopController.h` | 新增 `executeLoop()` / `buildLoopPrompt()` | 中 |
| `AgentLoopController.cpp` | 重构核心循环逻辑 | 中 |
| `AgentLoopPromptBuilder.h` | 新增 `buildLoopPrompt()` — 每步动态生成 | 低 |
| `AgentLoopPromptBuilder.cpp` | 实现单步 prompt 组装 | 低 |
| `AgentLoopActionParser.h` | 已有，无需改动 | — |
| `ApplicationController.h` | 新增 `runAgentLoop()` 入口 | 低 |
| `ApplicationController.cpp` | 统一模式使用新循环 | 低 |

#### AgentLoopPrompt 格式（新增 buildLoopPrompt）

```
你是 Codex Agent。你的任务是通过调用工具完成用户目标。

## 用户目标
{userGoal}

## 可用工具
{toolCatalog}

## 执行历史（最近 N 步）
Step 1: toolId=workspace.read_file, result=OK (1,203 bytes)
Step 2: toolId=json_format, result=OK (3,842 bytes)

## 当前状态
已完成 2 步，当前位于第 3 步。

## 指令
请分析当前状态，决定下一步操作。返回 JSON：

如果你认为任务已完成，返回：{"done": true, "summary": "任务完成摘要"}
如果你需要继续执行，返回：{"tool_id": "xxx", "reason": "为什么", "parameters": {...}}

只返回一行 JSON，不要加代码块标记。
```

#### 新增类：LoopTerminationPolicy

```cpp
struct LoopTerminationPolicy {
    int maxSteps = 10;                // 默认最大步数（V12 从 2 升到 10）
    int stepTimeoutMs = 30000;        // 单步超时（30s）
    int totalTimeoutMs = 300000;      // 总超时（5 分钟）
    bool allowRetryOnError = true;    // 工具失败是否重试
    int maxRetries = 2;               // 最大重试次数
};
```

#### 测试

```
tests/app/AgentLoopControllerV12Test.cpp（新增）
tests/app/AgentLoopPromptBuilderV12Test.cpp（新增）
```

| 测试用例 | 说明 |
|----------|------|
| `loop_terminatesOnDone` | AI 返回 done=true 时正常终止 |
| `loop_terminatesOnMaxSteps` | 达到步数上限时终止 |
| `loop_terminatesOnUserStop` | 用户停止时终止 |
| `loop_terminatesOnTimeout` | 超时时终止 |
| `loop_singleStepAction` | 单步 action 正确执行 |
| `loop_multiStepAction` | 多步 action 连续执行 |
| `loop_retryOnError` | 失败后重试 |
| `loop_promptIncludesHistory` | prompt 包含执行历史 |
| `loop_promptIncludesCurrentStep` | prompt 包含当前步数信息 |
| `loop_fallbackOnParseError` | 解析失败时降级处理 |

#### 验收标准

- ⬜ Agent 能连续执行 5+ 步不中断
- ⬜ 每一步的 prompt 包含上一步的执行结果
- ⬜ AI 返回 `done=true` 后自动终止
- ⬜ 超时自动终止，不卡死
- ⬜ 用户可随时停止
- ⬜ `ctest` 新增 10 个测试通过
- ⬜ 旧的 `AgentPlanExecutor` 保持向后兼容（`executePlan()` 仍可用）

---

### V12.3：流式工具执行（2 天）

#### 范围

参考 Claude Code 的 `StreamingToolExecutor`，在模型 streaming 输出期间就并行执行工具调用，不等待完整响应。

#### 当前流程

```
发送请求 → 等待全部响应 → 解析 tool_calls → 逐一执行工具
总延迟 = 模型输出时间 + N × 工具执行时间
```

#### 改造后流程

```
发送请求 → streaming 开始
  ├─ content_block (思考文本) → 实时显示
  ├─ content_block (tool_use_1) → 立即执行工具1
  ├─ content_block (tool_use_2) → 立即执行工具2（只读则并发）
  └─ message_stop
总延迟 ≈ max(模型输出时间, 工具执行时间)
```

#### 修改文件

| 文件 | 改动内容 |
|------|----------|
| `StreamParser.h` | 新增 `contentBlockStarted` / `contentBlockStopped` 信号 |
| `StreamParser.cpp` | SSE 解析协议改为分段回调模式 |
| `ApplicationController.cpp` | 新增 `handleContentBlock()` / `handleToolUseBlock()` |

#### 关键设计

```cpp
// StreamParser 新增信号
signals:
    void contentBlockStarted(const QString &blockType, const QString &blockId);
    void contentBlockDelta(const QString &blockId, const QString &delta);
    void contentBlockStopped(const QString &blockId);
    void toolUseBlockComplete(const QString &blockId, const QString &toolName,
                              const QJsonObject &arguments);
```

```
// ApplicationController 新增流式工具处理
void ApplicationController::handleToolUseBlockComplete(
    const QString &blockId, const QString &toolName,
    const QJsonObject &arguments)
{
    // 1. 在工具注册表中找到对应工具
    // 2. 立即开始执行（不等 message_stop）
    // 3. 结果缓存，在 message_stop 后统一拼入消息数组
    m_pendingToolResults.append({blockId, toolName, arguments, result});
}
```

#### 暂不做

- ❌ 并发工具执行（只读并发、写入串行）—— V12 资源不够，在 2 天内只做「提前启动执行」，串行即可
- ❌ Bash 错误级联取消 —— Codex 当前没有多个并行 Bash 命令的场景

#### 测试

```
tests/services/StreamParserV12Test.cpp（新增）
```

| 测试用例 | 说明 |
|----------|------|
| `blockStart_content` | 解析文本 content_block 开始事件 |
| `blockStart_toolUse` | 解析 tool_use content_block 开始事件 |
| `blockDelta_text` | 文本增量正确传递 |
| `blockDelta_toolArguments` | 工具参数增量累积 |
| `blockStop_toolUse` | 解析 tool_use 完成事件 |
| `multiBlock_singleMessage` | 单条消息含多个 content_block |
| `noToolCalls_plainText` | 纯文本消息正常处理 |
| `toolCallOnly_noText` | 只有工具调用的消息正常处理 |

#### 验收标准

- ⬜ 模型在 streaming 期间就能启动工具执行
- ⬜ 旧的非 streaming 路径保持向后兼容
- ⬜ `ctest` 新增 8 个测试通过
- ⬜ `StreamParserTest`（现有）继续通过

---

### 3.3 V12 验收标准汇总

| 维度 | 标准 |
|------|------|
| 测试 | `ctest` 新增 ≥ 26 个测试，全量通过（46 + 26 = 72） |
| 上下文 | 50 轮对话能正常发送，不报 413 |
| 循环 | Agent 能执行 5+ 步，AI 说 done 自动停止 |
| 流式 | 工具在模型输出期间就启动执行 |
| 回退 | 旧 `AgentPlanExecutor::executePlan()` 保持可用 |
| 文档 | 更新 `learn/01-architecture.md` |
| 无回归 | 现有 46 个测试继续通过 |

---

## 4. V13：记忆系统与技能增强

### 4.0 版本定位

> **一句话：让 Agent 有持久记忆、有可编程的 Hook 点、技能系统可以闭环运转。**

### 4.1 当前 vs 目标

| 能力 | 当前 | V13 后 |
|------|------|--------|
| 工作记忆 | ⚠️ `AGENT_MEMORY.md` 单文件，只支持追加 | ✅ 三层记忆 + 自动日志 |
| Skills | ⚠️ 外部 `.skill.md` 文件，只注入 prompt | ✅ Skills 可触发 Hook + 执行后自动日志 |
| Hook | ❌ 无 | ✅ 5 个核心 Hook 点 |

---

### V13.1：三层记忆系统（2 天）

#### 架构

```
~/.codex/MEMORY.md                 ← L1 用户级（跨项目偏好）
  ↓ 注入
.workbuddy/memory/MEMORY.md        ← L2 项目级（技术决策、约定）
  ↓ 注入
.workbuddy/memory/YYYY-MM-DD.md    ← L3 每日日志（自动追加、可检索）
```

#### 新增文件

```
src/memory/
├── ProjectMemoryManager.h        ← 统一记忆管理入口
├── ProjectMemoryManager.cpp
├── MemoryEntry.h                  ← 记忆数据结构
├── DailyMemoryWriter.h            ← 每日日志写入
└── DailyMemoryWriter.cpp
```

#### 核心接口

```cpp
class ProjectMemoryManager {
public:
    // 构建注入 Prompt 的记忆片段
    QString buildMemorySection(const QString &projectDir);

    // Agent 完成任务后自动追加日志（Agent 调用）
    void appendDailyLog(const QString &projectDir, const QString &category,
                        const QString &entry);

    // 用户明确说「记住」时写入
    void remember(const QString &projectDir, const QString &fact,
                  const QString &source = "user");

    // 读取最近 N 天的日志
    QVector<MemoryEntry> recentLogs(const QString &projectDir, int days);

    // 按关键词检索记忆
    QVector<MemoryEntry> search(const QString &projectDir,
                                 const QString &keyword);

    // 记忆安全校验
    static bool containsSensitiveContent(const QString &text);
};

struct MemoryEntry {
    QDateTime timestamp;
    QString category;       // "log" | "decision" | "convention" | "preference"
    QString content;
    QString source;         // "agent_auto" | "user_explicit" | "hook"
};
```

#### 修改文件

| 文件 | 改动 |
|------|------|
| `ApplicationController.cpp` | `onAgentLoopFinished` → 自动追加每日日志 |
| `AgentPlanPromptBuilder.cpp` | `buildUnifiedPrompt` → 注入三层记忆片段 |
| `AgentLoopPromptBuilder.cpp` | `buildLoopPrompt` → 注入项目记忆 |

#### 记忆安全规则

```
禁止保存的内容：
- 含 "api_key" / "token" / "password" / "secret" / "bearer" / "credential" 的文本
- 超过 1000 字符的单条记忆
- 二进制或非 UTF-8 内容
- 文件路径含 "C:\Windows\System32" 等系统目录的路径

自动截断：
- 单条记忆超过 500 字符时截断并标记 [truncated]
```

#### 测试

```
tests/memory/ProjectMemoryManagerTest.cpp（新增）
```

| 测试用例 | 说明 |
|----------|------|
| `buildMemorySection_allLayers` | 三层记忆正确拼接 |
| `buildMemorySection_missingFile` | 文件不存在时不崩溃 |
| `appendDailyLog_createsFile` | 自动创建日志文件 |
| `appendDailyLog_appends` | 追加而非覆盖 |
| `remember_persists` | 记忆持久化到文件 |
| `search_byKeyword` | 关键词检索 |
| `recentLogs_limit` | 按天数限制 |
| `sensitiveContent_apiKey` | 检测并拒绝 API Key |
| `sensitiveContent_password` | 检测并拒绝密码 |
| `sensitiveContent_token` | 检测并拒绝 token |

#### 验收标准

- ⬜ 三层记忆正确注入 prompt
- ⬜ Agent 任务完成后自动追加日志
- ⬜ 用户说「记住」后内容持久化到项目记忆
- ⬜ 敏感内容被自动拒绝
- ⬜ `ctest` 新增 10 个测试通过

---

### V13.2：Skills 增强 + Hook 系统（1.5 天）

#### 范围

在现有 V10.2 外部技能文件基础上，增加 5 个 Hook 点，让技能不仅仅是「注入 prompt」，而是可以拦截和修改工具行为。

#### Hook 点设计

```cpp
// src/app/AgentHookManager.h（新增）
// src/app/AgentHookManager.cpp（新增）

class AgentHookManager {
public:
    // Hook 1：工具调用前拦截
    // 技能可以：修改参数、拒绝执行、追加日志
    virtual bool beforeToolCall(const QString &toolId,
                                QJsonObject &params,
                                QString *rejectReason);

    // Hook 2：工具执行后处理
    // 技能可以：脱敏输出、截断结果、追加格式化
    virtual void afterToolCall(const QString &toolId,
                               ToolResult &result);

    // Hook 3：Prompt 构建前注入
    // 技能可以：追加系统提示、修改工具描述
    virtual QString beforePromptBuild(const QVector<AgentToolDescriptor> &tools);

    // Hook 4：Agent 循环结束后
    // 技能可以：自动追加记忆、生成总结、清理临时文件
    virtual void afterAgentLoop(const AgentLoopResult &result,
                                ProjectMemoryManager *memory);

    // Hook 5：会话切换时
    // 技能可以：注入会话级初始化指令
    virtual QString onSessionActivate(const QString &sessionId);

    // 从 skills 目录注册 Hook
    void registerSkillHooks(const QString &projectDir);

private:
    // 技能文件解析出的 Hook 配置
    struct SkillHook {
        QString skillName;
        QString hookPoint;      // "before_tool_call" | "after_tool_call" | ...
        QString targetToolId;   // 目标工具，* 表示所有
        QString instruction;    // 技能指令（注入 AI 或本地逻辑）
    };
    QVector<SkillHook> m_hooks;
};
```

#### Skills 文件格式增强

`.skill.md` 文件增加可选的 Hook 声明：

```markdown
---
name: code-review-helper
description: 代码审查后自动追加项目记忆
version: 1.0
hooks:
  - point: after_tool_call
    tool: git.review_diff
    action: |-
      将 diff 摘要追加到项目记忆，分类为 "code_review"
  - point: before_tool_call
    tool: command.*
    action: |-
      确保命令执行前有用户确认
  - point: after_agent_loop
    action: |-
      检查是否有未提交的代码变更，如有则追加提醒到日志
---

# 代码审查助手

## 描述
审查代码变更并记录到项目记忆。
```

#### 集成点

| 文件 | 改动 |
|------|------|
| `ApplicationController.cpp` | 初始化 `AgentHookManager`；在关键路径调用 Hook |
| `AgentCommandSkillFileService.cpp` | 解析 `.skill.md` 中的 `hooks` 字段 |
| `AgentToolRegistry.cpp` | `executeTool()` 前后调用 `beforeToolCall` / `afterToolCall` |

#### 测试

```
tests/app/AgentHookManagerTest.cpp（新增）
```

| 测试用例 | 说明 |
|----------|------|
| `beforeToolCall_modifyParams` | Hook 修改工具参数 |
| `beforeToolCall_reject` | Hook 拒绝工具调用 |
| `afterToolCall_redactSensitive` | Hook 脱敏工具输出 |
| `beforePromptBuild_injectContext` | Hook 注入 prompt 上下文 |
| `afterAgentLoop_autoMemory` | Hook 自动追加记忆 |
| `registerFromSkillFile` | 从 .skill.md 解析 Hook |
| `noHooks_noCrash` | 无 Hook 时不崩溃 |
| `multipleHooksSamePoint` | 同一 Hook 点多技能叠加 |

#### 验收标准

- ⬜ 5 个 Hook 点正常工作
- ⬜ Skills 文件可以注册 Hook
- ⬜ Hook 失败不影响主线执行
- ⬜ `ctest` 新增 8 个测试通过

---

### V13.3：V12/V13 整合验收（0.5 天）

#### 范围

- 全量测试运行
- 手工验证：长对话 → 自动裁剪 → Agent 多步执行 → 自动记忆
- 更新文档

#### 验收标准

| 维度 | 标准 |
|------|------|
| 测试 | `ctest` 新增 ≥ 18 个测试，全量 ≥ 90 个 |
| 记忆 | Agent 执行任务后 `YYYY-MM-DD.md` 自动新增条目 |
| Hook | skill.md 中的 Hook 在工具调用时触发 |
| 文档 | 更新 `README.md`、`learn/01-architecture.md` |

---

## 5. V14：电脑感知 → 操作闭环 MVP

### 5.0 版本定位

> **一句话：让 Agent 能「看」屏幕、「操作」电脑，形成完整的感知-行动闭环。**

V12/V13 做完后，Agent 的「大脑」（循环、上下文、记忆）已经就绪。V14 给大脑装上「眼睛」（感知）和「手」（操作）。

### 5.1 前置条件

- V12.2 无限 Agentic Loop ✅（Agent 能执行多步操作）
- V13.1 三层记忆 ✅（Agent 能记住之前的操作结果）
- V12 电脑感知占位代码存在 ← 需要重写为真实实现

### 5.2 现有 V12 代码的状态

| 文件 | 当前状态 | V14 改动 |
|------|----------|----------|
| `WindowDetector.cpp` | ✅ 完整（Win32 EnumWindows） | 接入 Hook 系统 |
| `ScreenCaptureService.cpp` | ✅ 完整（GDI BitBlt → BMP） | 接入 Agent Loop |
| `SystemInfoService.cpp` | ✅ 完整 | 接入 Agent Loop |
| `OcrService.cpp` | ❌ 占位（仅验证文件） | **重写为真实 OCR** |
| `UiAutomationService.cpp` | ❌ 占位 | **重写为真实 UIA** |
| `InputSimulator.cpp` | ❌ 占位 | **重写为真实 SendInput** |
| `ForegroundValidator.cpp` | ✅ 完整 | 无需改动 |

---

### V14.1：感知层接入 Agent Loop（2 天）

#### 目标

让 Agent 在循环中能调用感知工具，并把结果反馈给下一步决策。

#### 重写 OCR

```
src/tools/OcrService.cpp  ← 从占位 → Windows OCR API
```

**技术选型：** Windows.Media.Ocr（Windows 10+ 内置，无需额外安装）

```cpp
ToolResult OcrService::extractText(const QString &workspaceDirectory,
                                    const QString &imagePath) {
    // 1. 校验路径在工作目录内
    // 2. 加载图片为 SoftwareBitmap
    // 3. 调用 OcrEngine.RecognizeAsync()
    // 4. 提取所有 OcrLine → OcrWord → 拼接文本
    // 5. 返回 { text, confidence, lineCount }
}
```

#### 接入 Agent Loop

在 `AgentLoopController::executeLoop()` 的 prompt 中，为感知工具提供专门的描述：

```
## 感知工具（可以用来观察电脑状态）
- system.capture_screen: 截取当前屏幕 → 保存到工作目录
- system.ocr_text: 从截图图片提取文字
- system.list_windows: 枚举所有可见窗口
- system.foreground_window: 获取前台窗口标题

建议流程：
1. 先用 system.capture_screen 截图
2. 用 system.ocr_text 读取截图中的文字
3. 根据 OCR 结果决定下一步操作
```

#### 测试

```
tests/tools/OcrServiceTest.cpp（新增）
tests/integration/PerceptionLoopTest.cpp（新增，简单的截图→OCR→分析流程）
```

---

### V14.2：操作层接入 Agent Loop（2 天）

#### 重写 UI Automation + 输入模拟

```
src/tools/UiAutomationService.cpp  ← 从占位 → Windows UIAutomation API
src/tools/InputSimulator.cpp       ← 从占位 → SendInput API
```

**关键技术：**

| 功能 | API | 说明 |
|------|-----|------|
| 点击按钮 | `IUIAutomationElement::GetClickablePoint()` + `SendInput` | UIA 定位 + 低级输入 |
| 输入文本 | `SendInput(INPUT_KEYBOARD)` | 模拟键盘事件 |
| 查找元素 | `FindFirst(TreeScope_Descendants, condition)` | 按名称/AutomationId 查找 |

**安全强制规则：**

```cpp
// 任何操作前必须满足：
// 1. ForegroundValidator::validateForeground() 通过
// 2. 不在密码输入框操作（检测 UIA_IsPasswordPropertyId）
// 3. 不在 UAC 提升窗口操作
// 4. 不在系统关键窗口操作（Taskmgr、Regedit、Cmd as Admin）
```

#### 接入 Agent Loop

操作工具的 prompt 描述：

```
## 操作工具（可以用来操作电脑）
⚠️ 所有操作都必须先验证前台窗口！
⚠️ 不能操作密码输入框、UAC 弹出窗口。

- input.validate_foreground: 验证前台窗口
- input.click_button: 点击按钮（需要 UIA 定位先）
- input.type_text: 输入文本（不支持密码、API Key）
```

---

### V14.3：感知-操作闭环 Demo（1.5 天）

#### Demo 场景：打开记事本 → 输入文字 → 截图确认

```
用户: 帮我打开记事本，输入 "Hello from Codex"，然后截图给我看

Agent Loop:
Step 1: AI 分析 → 需要启动 notepad.exe
        → command.run(program="notepad.exe")
Step 2: 等待 1 秒 → system.list_windows
        → 找到 "无标题 - 记事本"
Step 3: system.foreground_window
        → 确认记事本在前台
Step 4: input.validate_foreground(expected="无标题 - 记事本")
        → 验证通过
Step 5: input.type_text(text="Hello from Codex")
        → SendInput 模拟键盘
Step 6: system.capture_screen
        → 截图保存
Step 7: system.ocr_text(image="screenshot.bmp")
        → 提取 "Hello from Codex"
Step 8: AI 判断 → done=true, summary="已打开记事本并输入文字，截图已验证"
```

#### 测试

```
tests/integration/DemoNotepadTest.cpp（新增，Windows 专用）
```

⚠️ 此测试需要真实 Windows 桌面环境，在 CI 中跳过。

---

### V14.4：V14 验收（0.5 天）

| 维度 | 标准 |
|------|------|
| OCR | 能从截图提取文字 |
| UIA | 能找到记事本的编辑区和按钮 |
| 输入 | SendInput 能模拟键盘输入 |
| 闭环 | 截图 → OCR → 验证 整条链跑通 |
| 安全 | 密码框/UAC 窗口操作被拒绝 |
| 测试 | `ctest` 新增 ≥ 6 个测试 |

---

## 6. V15：个人 AI 管家整合

### 6.0 版本定位

> **一句话：把聊天、Agent、角色、调度整合成一个完整的个人 AI 管家。**

### 6.1 子版本拆分

| 子版本 | 内容 | 预估 |
|--------|------|------|
| V15.1 | 调度层：Cron 定时器 + 任务队列 | 2 天 |
| V15.2 | 角色系统：多角色 + 情感状态 + 叙事上下文 | 3 天 |
| V15.3 | 使用统计面板 + 全文搜索聊天记录 | 1.5 天 |
| V15.4 | MCP 协议支持（工具生态互通） | 3 天 |

### 6.2 V15 详细规划

> 暂缓——等 V12-V14 的核心基础设施稳定后再展开。

---

## 7. 版本间依赖关系

```
V12.1 上下文管理 ────────┐
                          ├─→ V13.1 记忆系统
V12.2 无限循环 ──────────┤       │
                          │       ├─→ V13.2 Hook 系统
V12.3 流式工具 ──────────┘       │
                                  ↓
                            V14.1 感知层
                                  │
                                  ├─→ V14.2 操作层
                                  │       │
                                  └───────┴─→ V14.3 闭环 Demo
                                                │
                                                ↓
                                          V15 管家整合
```

---

## 8. 里程碑与交付物

| 里程碑 | 完成标准 | 预计日期 |
|--------|----------|----------|
| M1: V12 验收 | 72 个测试通过，上下文+循环+流式就绪 | V12 + 6 天 |
| M2: V13 验收 | 90 个测试通过，记忆+Hook 就绪 | V13 + 4 天 |
| M3: V14 验收 | 96 个测试通过，感知-操作闭环跑通 | V14 + 6 天 |
| M4: V15 启动 | 基础设施稳定后评估 | 待定 |

---

## 9. 简历表达（V14 完成后）

> 在 C++/Qt 桌面应用中设计并实现了**完整 Agent 系统**，包含：
> - **无限 Agentic Loop**（观察→决策→行动→再观察），支持 AI 动态选择工具和终止判断
> - **自动上下文管理**（Token 估算 + 智能裁剪 + AI 摘要注入），长对话不报错
> - **三层跨会话记忆**（全局/项目/每日日志），Agent 任务自动记录
> - **可编程 Hook 插件系统**（5 个拦截点），技能文件可注册行为
> - **电脑感知-操作闭环**：截图 + OCR 识别 + UI Automation 定位 + SendInput 模拟
> - 40+ 内置工具，90+ 自动化测试，100% 通过
> - 安全设计：前台窗口校验、密码框/UAC 拦截、工作目录隔离、白名单命令执行

---

## 10. 文档索引

- [优化方向.md](优化方向.md) — 第九节：主流 Agent 系统深度对标分析
- [39-v11-plus-development-roadmap.md](39-v11-plus-development-roadmap.md) — V11 旧版规划（已完成）
- [40-v11-tool-ecosystem-design.md](40-v11-tool-ecosystem-design.md) — V11 工具生态设计
- [41-v12-computer-awareness-design.md](41-v12-computer-awareness-design.md) — V12 旧版电脑感知设计（部分过时，以本文档为准）
- [43-v11-acceptance-notes.md](43-v11-acceptance-notes.md) — V11 验收记录
