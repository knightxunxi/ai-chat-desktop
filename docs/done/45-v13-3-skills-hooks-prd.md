# V13.3 Skills + Hooks 系统 — 产品需求文档

**文档状态**: 待评审  
**创建日期**: 2026-06-04  
**关联版本**: CodeXX V13.3  
**前置依赖**: V12 (Agent 基础设施), V13.1 (三层记忆), V13.2 (记忆压缩)

---

## 1. 项目信息

| 字段 | 值 |
|------|-----|
| Language | 中文 |
| Programming Language | C++17 / Qt 6 Widgets |
| Project Name | codexx_v13_3_skills_hooks |
| 原始需求 | 为 CodeXX 桌面应用添加可扩展的 Skills 技能系统和 Hooks 生命周期钩子系统，让 Agent 能根据用户输入动态匹配技能指令，并在关键节点插入可配置回调。 |

---

## 2. 产品定义

### 2.1 Product Goals

1. **可扩展的技能生态**：用户和项目可独立定义技能文件，Agent 自动匹配并注入指令，无需修改 C++ 源码即可扩展 Agent 能力。
2. **可编程的生命周期钩子**：在 Agent 循环 6 个关键节点提供拦截点，支持内置 C++ 钩子和外部脚本钩子（Python/Bash）。
3. **与现有 Agent 循环无缝集成**：Skills 匹配结果注入 `AgentLoopPromptBuilder`，Hooks 嵌入 `AgentLoopController` 和 `ApplicationController`，不破坏现有 V12 架构。

### 2.2 User Stories

| # | 场景 |
|---|------|
| US-1 | **As a** 开发者, **I want** 在项目 `.workbuddy/skills/` 目录下放置一个 `SKILL.md` 文件定义代码审查流程, **so that** 当我说"审查代码"时 Agent 自动加载该流程指令。 |
| US-2 | **As a** 高级用户, **I want** 在 `~/.codex/skills/` 下定义个人通用技能（如"技术博客写作"）, **so that** 所有项目都能复用该技能。 |
| US-3 | **As a** 开发者, **I want** 在 Agent 发送请求前自动注入当前时间戳（内置 Hook）, **so that** LLM 始终知道当前时间而无需每次手动告知。 |
| US-4 | **As a** 工具开发者, **I want** 编写 Python 脚本在工具执行后自动格式化输出（脚本 Hook）, **so that** 无需修改 C++ 源码就能定制工具行为。 |
| US-5 | **As a** 项目维护者, **I want** 项目级技能优先级高于用户级同名技能, **so that** 项目约定的工作流不会被个人偏好覆盖。 |

---

## 3. 技术规范

### 3.1 Requirements Pool

#### P0 — 必须实现（核心闭环）

| ID | 需求 | 说明 |
|----|------|------|
| **SK-P0.1** | Skills 文件发现与解析 | 扫描 `~/.codex/skills/`（用户级）和 `.workbuddy/skills/`（项目级），解析 `SKILL.md` 文件中的 YAML frontmatter（name, description, triggers, priority）和 Markdown 指令体 |
| **SK-P0.2** | 关键词触发匹配 | 根据用户输入与技能 triggers 列表做子串/正则匹配，匹配到的技能按优先级排序 |
| **SK-P0.3** | System Prompt 注入 | 匹配到的技能指令注入 `AgentLoopPromptBuilder::buildNextActionPrompt()` 的 system prompt 中，放在工具定义之后、用户目标之前 |
| **SK-P0.4** | 技能优先级管理 | 项目级 > 用户级同名技能（按 name 字段判重）；同级别按 priority 数值排序 |
| **HK-P0.1** | 6 个 Hook 点定义与调用 | `pre_send` → `post_receive` → `on_tool_execute`（前/后） → `on_error` → `on_agent_start` → `on_agent_stop`，在 `AgentLoopController` 和 `ApplicationController` 中埋点 |
| **HK-P0.2** | 内置 Hook 机制 | C++ 硬编码的内置 Hook 基类 + 注册机制，首批内置 Hook：时间戳注入（pre_send）、速率限制（pre_send）、敏感信息过滤（post_receive） |
| **HK-P0.3** | 脚本 Hook 执行 | 通过 `QProcess` 调用外部 Python/Bash 脚本，stdin 传入 JSON 上下文，stdout 读取修改后的上下文/结果；超时 10 秒，超时则跳过并记录警告 |

#### P1 — 应该实现（增强体验）

| ID | 需求 | 说明 |
|----|------|------|
| **SK-P1.1** | Skills 热重载 | 文件系统监控（`QFileSystemWatcher`）检测 skills 目录变化，自动重新加载，无需重启应用 |
| **SK-P1.2** | 技能匹配日志 | 在应用日志中记录每次技能匹配结果（匹配了哪些技能、为什么匹配），便于用户调试技能触发逻辑 |
| **SK-P1.3** | 技能列表 UI 展示 | 在设置窗口新增「技能」标签页，展示已加载的技能列表、来源、状态 |
| **HK-P1.1** | Hook 执行日志 | 记录每次 Hook 的执行结果（成功/超时/拒绝），写入应用日志 |
| **HK-P1.2** | 脚本 Hook 白名单 | 脚本 Hook 执行前校验脚本路径在允许的目录内（`~/.codex/hooks/` 或 `.workbuddy/hooks/`），拒绝执行任意路径脚本 |

#### P2 — 可以后续实现

| ID | 需求 | 说明 |
|----|------|------|
| **SK-P2.1** | 技能参数化 | 支持 `triggers` 中使用 `{param}` 占位符，提取用户输入中的参数注入技能指令 |
| **SK-P2.2** | 技能市场/分享 | 支持从远程 URL 安装技能包 |
| **HK-P2.1** | Hook 链式编排 | 同一 Hook 点支持多个 Hook 的顺序执行和前一个输出作为后一个输入的管道模式 |
| **HK-P2.2** | Hook UI 配置界面 | 在设置窗口提供 Hook 启用/禁用开关和脚本路径配置 |

### 3.2 UI Design Draft

本次为后端基础设施，UI 改动量小。

**设置窗口 → 新增「技能」标签页**（P1）：
```
┌──────────────────────────────────────────┐
│ 设置                                      │
│ [API] [模型] [语言] [技能] [Hook]         │
├──────────────────────────────────────────┤
│ 已加载技能 (3)                            │
│ ┌──────────────────────────────────────┐  │
│ │ ✅ code-review-helper  (项目级) v1.0  │  │
│ │    触发词: 审查代码, code review      │  │
│ │ ✅ git-commit-assist   (用户级) v1.0  │  │
│ │    触发词: 提交代码, git commit       │  │
│ │ ⚠️ python-debugger    (禁用) v0.1    │  │
│ │    触发词: 调试, debug                │  │
│ └──────────────────────────────────────┘  │
│ 技能目录:                                  │
│   ~/.codex/skills/          [打开目录]     │
│   .workbuddy/skills/        [打开目录]     │
└──────────────────────────────────────────┘
```

### 3.3 关键架构集成点

```
用户输入 → ApplicationController::sendAgentLoopMessage()
   │
   ├─ [HK] on_agent_start → AgentLoopController::executeLoop()
   │     │
   │     ├─ [HK] pre_send → ApplicationController::continueAgentLoop()
   │     │     └─ AgentLoopPromptBuilder::buildNextActionPrompt()
   │     │           ├─ 注入工具定义
   │     │           ├─ 注入三层记忆 (V13.1)
   │     │           ├─ [SK] 注入匹配技能指令 ← NEW
   │     │           └─ 注入用户目标
   │     │
   │     ├─ AI 请求/响应 (OpenAICompatibleClient)
   │     │
   │     ├─ [HK] post_receive → handleRequestFinished()
   │     │
   │     ├─ [HK] on_tool_execute (before) → AgentToolRegistry::executeTool()
   │     ├─ 工具执行
   │     ├─ [HK] on_tool_execute (after)  → AgentToolRegistry::executeTool()
   │     │
   │     ├─ [HK] on_error → handleRequestFailed() / 工具执行失败
   │     │
   │     └─ 循环直到终止
   │
   └─ [HK] on_agent_stop → AgentLoopController 返回结果
```

### 3.4 SKILL.md 文件格式规范

```markdown
---
name: code-review-helper           # 必填，唯一标识
description: 代码审查助手           # 必填，一句话描述
version: "1.0"                    # 必填，语义化版本
triggers:                          # 必填，触发关键词列表
  - 审查代码
  - review code
  - code review
  - CR
priority: 10                       # 选填，优先级 0-100，默认 0
enabled: true                      # 选填，默认 true
author: your-name                  # 选填
---

# 代码审查助手

## 触发条件
当用户请求代码审查、code review、CR 时激活。

## 执行流程
1. 使用 `git.diff_uncommitted` 获取未提交的变更
2. 逐文件分析代码质量、潜在 bug、命名规范
3. 使用 `git.review_diff` 对每个文件生成审查意见
4. 汇总所有审查意见，按严重程度排序（🔴严重 → 🟡建议 → 🟢优化）

## 注意事项
- 关注安全漏洞（SQL 注入、XSS、硬编码密钥）
- 遵循项目 AGENT.md 中的代码规范
- 审查结果追加到项目记忆（分类: code_review）
```

**格式约束**：
- 文件编码：UTF-8
- 文件大小上限：32 KB（沿用 `DefaultMaxSkillFileBytes`）
- 项目级最多加载 20 个技能文件（沿用 `DefaultMaxSkillFiles`）
- YAML frontmatter 必须以 `---` 开始和结束
- Markdown 指令体不可为空（最少 50 字符）

### 3.5 Hook 脚本 JSON 协议

**stdin 输入格式**：
```json
{
  "hook_point": "pre_send",
  "context": {
    "session_id": "abc-123",
    "project_dir": "/path/to/project",
    "user_message": "帮我审查代码",
    "prompt": "完整的 system prompt + 用户消息..."
  },
  "metadata": {
    "timestamp": "2026-06-04T10:30:00Z",
    "agent_iteration": 3,
    "tool_id": null
  }
}
```

**stdout 期望输出**：
```json
{
  "action": "modify",
  "modified_context": {
    "prompt": "修改后的 prompt（追加了时间戳）"
  }
}
```

或：
```json
{
  "action": "pass"
}
```

或：
```json
{
  "action": "reject",
  "reason": "检测到敏感操作，已拦截"
}
```

**安全约束**：
- 脚本进程超时：10 秒
- stdout 最大读取：64 KB
- 脚本执行目录隔离：`QProcess::setWorkingDirectory()` 指向临时目录
- 环境变量白名单：只传递 `PATH`, `HOME`, `USERPROFILE`, `TEMP`
- 不传递任何 API Key 或凭据到脚本环境

---

## 4. Open Questions

| # | 问题 | 优先级 |
|---|------|--------|
| Q1 | Skills 触发是否需要支持正则表达式？当前设计为子串匹配，正则更灵活但安全风险更高 | P1 |
| Q2 | 脚本 Hook 是否需要支持 Node.js？当前设计 Python/Bash，用户可能偏好 Node | P2 |
| Q3 | 内置 Hook 的启用/禁用是否需要持久化配置？当前设计编译时固定 | P1 |
| Q4 | ~~Skills 匹配结果是否需要用户确认后才注入 prompt？还是自动注入？~~ **已确认**：自动注入 + 日志记录。每轮 Agent 循环完成后在聊天中显示技能使用总结，过程信息简略展示在状态栏/日志区。 | ✅ P0 |
| Q5 | 用户级 skills 目录 `~/.codex/skills/` 是否需要与 WorkBuddy 共享技能文件？格式兼容但目录结构不同 | P2 |
| Q6 | ~~脚本 Hook 超时后是否应该阻塞 Agent 循环还是跳过继续？~~ **已确认**：跳过 + 警告日志，不阻塞 Agent 循环。未来可探索重试/降级方案。 | ✅ P0 |

---

## 5. 与现有模块的改动清单

| 现有文件 | 改动类型 | 说明 |
|----------|----------|------|
| `src/app/AgentCommandSkillFileService.h/.cpp` | **重构** | 升级为 `SkillFileService`：新增 YAML frontmatter 解析、双目录扫描、优先级合并 |
| `src/app/AgentLoopPromptBuilder.h/.cpp` | **修改** | `buildNextActionPrompt()` 新增 `matchedSkills` 参数 |
| `src/app/AgentLoopController.h/.cpp` | **修改** | `executeLoop()` 中嵌入 Hook 调用点 |
| `src/app/ApplicationController.h/.cpp` | **修改** | 新增 `SkillManager` 和 `HookManager` 成员；`sendAgentLoopMessage()` 中初始化技能匹配 |
| `src/tools/AgentToolRegistry.h/.cpp` | **修改** | `executeTool()` 前后嵌入 on_tool_execute Hook |

| 新增文件 | 说明 |
|----------|------|
| `src/skills/SkillDefinition.h` | 技能数据结构（YAML 元数据 + Markdown 指令体） |
| `src/skills/SkillManager.h/.cpp` | 技能发现、加载、解析、匹配、优先级管理 |
| `src/skills/SkillFileParser.h/.cpp` | YAML frontmatter + Markdown 体解析器 |
| `src/hooks/HookDefinition.h` | Hook 点枚举、Hook 基类、HookResult 结构 |
| `src/hooks/HookManager.h/.cpp` | Hook 注册、分发、执行协调 |
| `src/hooks/BuiltinHooks.h/.cpp` | 内置 Hook 实现（时间戳注入、速率限制、敏感信息过滤） |
| `src/hooks/ScriptHookRunner.h/.cpp` | QProcess 脚本执行器（沙箱、超时、JSON 协议） |
| `tests/skills/SkillManagerTest.cpp` | 技能系统测试 |
| `tests/hooks/HookManagerTest.cpp` | Hook 系统测试 |

---

## 6. 验收标准

| # | 标准 | 对应需求 |
|---|------|----------|
| AC-1 | `~/.codex/skills/` 和 `.workbuddy/skills/` 下的 `SKILL.md` 被正确发现和解析 | SK-P0.1 |
| AC-2 | 用户输入匹配到技能 triggers 后，技能指令出现在 Agent system prompt 中 | SK-P0.2, SK-P0.3 |
| AC-3 | 项目级同名技能覆盖用户级 | SK-P0.4 |
| AC-4 | 6 个 Hook 点在 Agent 循环中正常触发 | HK-P0.1 |
| AC-5 | 内置「时间戳注入」Hook 在 pre_send 阶段正确拼接当前时间 | HK-P0.2 |
| AC-6 | 脚本 Hook 正常执行，stdin/stdout JSON 协议正确 | HK-P0.3 |
| AC-7 | 脚本 Hook 超时（10s）后跳过并记录警告，不影响主线 | HK-P0.3 |
| AC-8 | 恶意脚本路径（如 `C:\Windows\System32\calc.exe`）被拒绝执行 | HK-P1.2 |
| AC-9 | `ctest` 新增 ≥ 16 个测试，全量测试通过 | — |
| AC-10 | 现有 V12 功能无回归（上下文管理、Agent 循环、流式工具执行） | — |
