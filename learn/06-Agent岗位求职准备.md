# CodeXX Agent 岗位求职准备

> 基于 CodeXX 项目（约 3 万行 C++/Qt6，68 个 CTest，12 个 CMake 静态库）
> 目标岗位：AI Agent 开发工程师 / LLM应用开发 / 智能体引擎开发

---

## 一、Agent 岗位市场现状

### 1.1 谁在招 Agent 岗位？

| 公司类型 | 代表 | 岗位侧重 | 薪资范围（成都参考） |
|----------|------|----------|:---:|
| **大厂 AI Lab** | 字节 Coze、阿里百炼、腾讯元器 | Agent 平台搭建、工作流引擎 | 20-40K |
| **AI 创业公司** | Dify、FastGPT、扣子 | Agent 编排、插件系统 | 15-30K |
| **垂直行业** | 金融量化、医疗问诊、法律咨询 | 行业 Agent + RAG + Function Calling | 12-25K |
| **传统软件厂** | 用友、金蝶、各类 SaaS | AI Copilot 嵌入现有产品 | 10-20K |

### 1.2 Agent 岗位 JD 高频关键词

```
Function Calling / Tool Use    — 90% 的岗位都会提
RAG（检索增强生成）             — 70% 的岗位会提
Multi-Agent / 多智能体协作      — 50% 的岗位会提
Prompt Engineering / 提示工程   — 80% 的岗位会提
MCP（Model Context Protocol）  — 2025后热门，40% 岗位
Agent Loop / ReAct / Plan-Execute — 60% 会提
LangChain / AutoGen / CrewAI   — Python 栈常提
Sidecar / JSONL / JSON-RPC     — 混合语言 Agent 架构常见
流式输出 / SSE                  — 基础能力，普遍要求
记忆系统 / Memory              — 30% 会提
安全沙箱 / 工具权限控制         — 大厂/金融看重
```

### 1.3 你的 CodeXX 项目覆盖了哪些？

| JD 关键词 | CodeXX 对应实现 | 覆盖度 |
|-----------|----------------|:---:|
| Function Calling | OpenAICompatibleClient + StreamParser 流式解析 tool_calls + 函数名→本地工具映射 | ✅ 全链路 |
| RAG | ProjectMemoryManager + 三层记忆注入 + 14天窗口检索 | ✅ 记忆增强 |
| MCP 协议 | McpRegistry + McpConnector，JSON-RPC over QProcess | ✅ 完整实现 |
| Agent Loop | AgentOrchestrator 50轮OODA循环 + 重复检测 + 上下文压缩 | ✅ 生产级 |
| Prompt Engineering | AgentLoopPromptBuilder + 意图分类 + 工具排序引导 | ✅ 多段拼接 |
| 流式输出 | SSE StreamParser + Delta 增量更新 + [DONE] 检测 | ✅ 全链路 |
| 记忆系统 | L1用户偏好/L2项目约定/L3每日日志 + ISO周压缩 + 敏感过滤 | ✅ 三层设计 |
| 安全沙箱 | 三级风险(Low/Medium/High) + 命令白名单 + 路径穿越防护 + 系统窗口黑名单 | ✅ 纵深防御 |
| 桌面自动化 | SendInput 键鼠模拟 + UIAutomation + OCR + 截图 + 前台窗口校验 | ✅ Win32原生 |
| 工具注册表 | AgentToolRegistry，35+工具，7类，单一数据源，Lambda执行体 | ✅ 教科书级 |
| Hook插件 | 6个Hook点 + BuiltinHook + ScriptHookRunner + QProcess沙箱 | ✅ 可扩展 |
| Python 能力层 | Python sidecar + JSONL 协议 + C++ QProcess 客户端 | ✅ V19 已启动 |

---

## 二、Agent 岗位简历优化

### 2.1 简历标题建议

根据投递方向选择：

```
通用C++开发:         C++开发工程师 | 23K行 Qt6/C++17 AI桌面应用
Agent方向:           AI Agent开发工程师 | C++17/Qt6 自研Agent平台
LLM应用方向:         LLM应用开发 | C++ AI Agent + Function Calling
```

### 2.2 Agent岗位专用简历描述

```text
CodeXX AI Agent 桌面平台 | C++17 / Qt6 / CMake / OpenAI API / Win32

核心能力：
- 独立设计并实现 Agentic Loop 连续执行引擎，支持 50 轮 OODA 循环，含意图分类、
  工具排序、重复动作检测（指纹表）、上下文溢出压缩（Microcompact + Reactive）、
  输出截断续接能力。
- 实现 OpenAI-compatible Function Calling 全链路：StreamParser 流式解析
  tool_calls 增量 JSON → 函数名映射 → 35+ 本地工具调度执行，支持 Chat 和 Agent
  双模式。
- 设计三层记忆系统（用户偏好/项目约定/每日日志 + ISO 周压缩 + LLM 摘要），
  14 天窗口 + 30K 硬上限，自动注入 Agent 提示词上下文。
- 实现 Skills+Hooks 插件体系：SkillManager 双目录扫描 + 子串匹配 + 优先级合并；
  HookManager 6 个钩子点（循环前/后、工具前后、请求前后）+ 安全过滤器。
- 桌面自动化闭环：Win32 SendInput 键鼠模拟 + UIAutomation 控件定位 +
  Windows.Media.Ocr OCR + 前台窗口黑名单安全校验。
- 集成 MCP 协议支持，JSON-RPC over QProcess 连接外部工具服务器。
- 68 个 CTest 自动化测试零回归，12 个 CMake 静态库分层架构，并启动 Python sidecar 能力层演进。
```

### 2.3 一句话项目描述（不同场合）

| 场合 | 推荐表述 |
|------|----------|
| 简历标题下 | "从零独立开发的 C++ AI Agent 桌面平台，实现 Function Calling、MCP 协议、多步推理循环、三层记忆系统" |
| 自我介绍 30秒 | "我用 C++/Qt6 独立做了一个 AI Agent 桌面应用，核心是让 AI 能像人一样操作电脑——截图→OCR识别→鼠标点击→键盘输入，整个过程全自动闭环" |
| 技术面开场 | "这个项目最核心的是 Agent Loop 执行引擎，实现了完整的 OODA 循环——AI 分析当前状态、选择一个工具执行、观察结果、再决定下一步，直到任务完成" |

---

## 三、Agent 面试高频考点

### 3.1 "什么是 Agent？和普通 LLM 有什么区别？"

> Agent 是能自主规划、使用工具、观察结果、迭代执行的 LLM 应用。普通 LLM 是"我问你答"，Agent 是"给我一个目标，我自己想办法完成"。
>
> 我在 CodeXX 中实现了这个区别：Chat 模式就是普通 LLM——用户问、AI 答；Agent 模式则完全不同——用户说"帮我改代码"，AI 会先 `file.read_text` 读文件，然后 `file.edit_text` 改代码，再 `command.bash` 编译测试，失败了还会自动修复。这一整套"感知→决策→执行→观察→再决策"的循环，就是 Agent 的核心。

### 3.2 "ReAct 和 Function Calling 的关系是什么？"

> ReAct（Reasoning + Acting）是一种思路：让 AI 在每一步都先推理、再行动、再观察。Function Calling 是具体的技术实现——OpenAI 定义了一种格式让模型输出结构化的函数调用指令。
>
> 代码层面：ReAct 是 AgentOrchestrator 的 executeLoop()——它管理整个"推理→执行→观察"的循环；Function Calling 是 OpenAICompatibleClient 发送 tools schema 并接收 tool_calls 的通道。两者是"思想"和"工具"的关系。

### 3.3 "怎么设计一个安全的 Agent 工具系统？"

> 我做了五层安全：
>
> 1. **风险分级**：每个工具有 Low/Medium/High 三级，AI 不能降低风险等级
> 2. **命令白名单**：command.* 工具只能用固定模板（如 `git status --short --branch`），不接受 AI 自定义参数
> 3. **工作目录沙箱**：文件操作限制在指定工作目录内，路径穿越自动拒绝
> 4. **系统窗口黑名单**：桌面操作前验证前台窗口，禁止操作 Task Manager/UAC/Ctrl+Alt+Del
> 5. **输出过滤**：SensitiveFilterHook 正则检测，拒绝 API Key/Token/Password 等敏感内容从工具结果泄露

### 3.4 "Agent 陷入死循环怎么办？"

> 三道防线：
> - **重复动作指纹检测**：同 toolId + 同参数出现 3 次 → 终止。维护 QSet<QString> 指纹表
> - **上下文溢出压缩**：Microcompact 将早期 observation 压缩为摘要（保留最近 5 条完整）
> - **硬上限**：最大 50 轮，超限强制终止并输出当前进度

### 3.5 "MCP 协议你是怎么实现的？"

> MCP 核心是 JSON-RPC 2.0 over stdio。我的 McpConnector 启动外部工具进程，通过 QProcess 读写 stdin/stdout，发送 JSON-RPC 请求获取工具列表和调用工具。McpRegistry 管理多个连接器，把 MCP 工具统一转为 AgentToolDefinition 注册到工具表。这样 Agent 看到的工具来源是统一的，不管它是本地实现的还是通过 MCP 从外部服务器获取的。

### 3.6 "StreamParser 解析 Function Calling 的难点是什么？"

> Function Calling 的 tool_calls 是增量返回的——第一个 chunk 可能只有 `{"index":0,"id":"call_xxx"`，第二个 chunk 才是 `,"function":{"name":"file.read_text"`。所以不能每收到一个 Delta 就尝试解析完整 JSON，因为它一定是不完整的。
>
> 我的方案是：StreamParser 内维护一个 JSON 累积缓冲区，每收到 tool_calls 增量就追加，然后用 `QJsonDocument::fromJson()` 尝试解析——作为"完整性探测器"。解析成功 → 提取 toolCall；解析失败 → 静默等待更多数据。这样避免了手写 JSON 状态机。

### 3.7 "Context Window 管理怎么做？"

> 我用了一个三级策略：
> 1. **阈值触发**：总 token 超过模型窗口的 85% 时触发压缩
> 2. **摘要压缩**：保留最近 3 轮完整对话，更早的通过 SummaryAPIClient 调用 AI 生成摘要
> 3. **兜底排序**：ensureRoleOrdering() 保证消息列表符合 API 要求（user/assistant 交替，最后一条必须是 user 或 tool）
>
> 模型窗口映射：deepseek-chat 64K / deepseek-v4-flash 128K。

---

## 四、STAR 故事（Agent专属版）

### STAR 1：从 20 工具到 51 工具的架构演进

**S**：Agent 功能迭代了 10 个版本后，工具从最初的 5 个膨胀到 35+ 个。每次加工具要改 PromptBuilder、Parser、Executor 三处。

**T**：统一工具管理，让新增工具只需 10 行代码。

**A**：
- 设计了 AgentToolRegistry（注册表模式），每个工具一条 lambda 定义
- Prompt 中的工具目录、Function Calling schema、执行器全部从同一份 Registry 生成
- 把 762 行的 defaultRegistry() 拆为 9 个类别化注册函数（registerFileTools/registerPerceptionTools...）
- tools/ 目录拆为 7 个 CMake 子库按领域独立编译

**R**：新增工具从"改 3 个文件"变成"Registry 里加 10 行"，68 个测试零回归。面试时可以直接展示代码：`REGISTER_TOOL("file.read_text", ..., [](const QJsonObject &args) { ... });`

### STAR 2：ApplicationController 上帝对象拆分

**S**：ApplicationController 膨胀到 350 行头文件 + 1600 行实现，配置管理、会话持久化、Agent 循环、Skills/Hooks/MCP 全部耦合。

**T**：按单一职责原则拆分，零外部行为变更。

**A**：
- 拆出 ConfigCoordinator（配置+Prompt模板，96 行）
- 拆出 SessionCoordinator（会话生命周期+持久化，514 行）
- 拆出 AgentOrchestrator（Agent 循环+Skills/Hooks/MCP，632 行）
- AC 从 350→220 行（-37%），.cpp 从 1600→1270 行（-21%）
- Coordinator signals → AC connect 转发 → MainWindow，上层零改动

**R**：68 个测试全部通过，零回归。架构师的角色在这里体现——不是"能跑就行"，而是"怎么跑得更好维护"。

### STAR 3：Agent 会话恢复

**S**：Agent 执行 30 轮后程序崩溃，之前 29 轮的执行结果全部丢失。

**T**：实现崩溃/重启后的 Agent 执行恢复。

**A**：
- 每步执行后自动持久化 AgentLoopState 到 JSON 文件
- 启动时检测 agent_state.json 是否存在，弹出恢复对话框
- 恢复时将之前所有 observations 和工具执行结果注入上下文
- Agent 从断点继续执行，用户无感知

**R**：Agent 循环从"一次性"变成"可恢复"，提升了生产级的可靠性。

### STAR 4：C++ 主控 + Python 能力层演进

**S**：项目的 UI、Agent 循环、工具执行和桌面自动化都已在 C++/Qt 中稳定运行，但后续多厂商 SDK、精确 tokenizer、embedding、网页抽取、文档解析、Playwright 浏览器自动化更适合 Python 生态。

**T**：在不重写 Agent 主循环的前提下，引入 Python 能力层，为后续 AI 能力扩展留出架构空间。

**A**：
- 保留 C++ 作为主控层，继续负责 UI、会话、Agent 状态机、工具注册和权限边界。
- 新增 Python sidecar，通过 stdin/stdout 上的 JSONL 协议提供 `ping`、`token.count`、`model.chat` 等能力。
- C++ 新增 `PythonSidecarProtocol` 和 `PythonSidecarClient`，用 `QProcess` 管理子进程、请求超时、stderr 和结构化错误。
- 新增 Python 单元测试、C++ 协议测试和 C++ 启动真实 Python sidecar 的集成测试。

**R**：Python 能力层已能独立运行并被 C++ 调用，CTest 演进到 68/68 通过；后续可在不破坏 C++ Agent 主循环的情况下接入 tokenizer、WebSearch、文档解析和 Playwright。

---

## 五、岗位匹配度自查表

| 岗位方向 | CodeXX 匹配度 | 需要补的知识 |
|----------|:---:|------|
| AI Agent 开发（C++） | ⭐⭐⭐⭐⭐ 95% | 几乎全部覆盖 |
| AI Agent 开发（Python） | ⭐⭐⭐⭐ 75% | Python sidecar 已启动，后续补 tokenizer/embedding/Web/Playwright |
| LLM 应用开发 | ⭐⭐⭐⭐ 80% | RAG 专项（向量数据库/Milvus） |
| 后端开发（C++） | ⭐⭐⭐ 65% | 网络编程/多线程/分布式 |
| 桌面应用开发（Qt） | ⭐⭐⭐⭐⭐ 90% | CodeXX 本身就是 Qt 项目 |

### 投递策略建议

| 优先级 | 岗位类型 | 原因 |
|:---:|------|------|
| P0 | **C++ Agent/LLM应用** | 项目完美匹配，竞争力最强 |
| P0 | **AI 平台工具开发** | Function Calling/MCP 是核心能力 |
| P1 | **Qt 桌面开发** | 技术栈完全匹配 |
| P1 | **通用 C++ 后端** | 项目体现工程能力 |
| P1 | **Python AI 开发** | V19 已开始 Python 能力层，重点补 tokenizer、embedding、Web/文档处理 |

---

## 六、面试前的自我检查清单

### 必须能解释清楚

- [ ] Agent Loop OODA 循环的 5 个阶段
- [ ] Function Calling 从发送 tools 到执行工具的完整链路
- [ ] StreamParser 如何处理不完整的 JSON tool_calls
- [ ] 工具风险三级的定义和权限降级保护
- [ ] 命令白名单 vs 任意命令执行的安全边界
- [ ] 记忆系统的三层设计和压缩策略
- [ ] 上下文窗口管理的压缩触发条件
- [ ] AutoFix 闭环（编辑→编译→测试→修复）
- [ ] Python sidecar 为什么只做能力层、不接管 Agent 主循环

### 必须能写出来（白板）

- [ ] AgentToolRegistry 的注册表数据结构
- [ ] Agent 循环的核心伪代码
- [ ] Function Calling tools schema 的 JSON 格式
- [ ] 一个完整工具条目的定义（ID+描述+参数+执行函数）

### 面试中主动引导

当你回答完一个问题后，可以自然引出更深的话题：
- 回答了 "Function Calling 怎么实现" → 接着说 "顺便一提，我还解决了 tool_calls JSON 增量解析的问题..."
- 回答了 "Agent 循环" → 接着说 "我还做了重复动作检测和上下文溢出压缩..."
- 回答了 "安全设计" → 接着说 "我还实现了 Hook 系统，可以在 6 个点插入安全检查..."

---

## 七、文档索引

| 需求 | 查看文档 |
|------|----------|
| 想了解项目整体架构 | [01-architecture.md](01-architecture.md) |
| 想看 26 个业务流程细节 | [02-key-flows.md](02-key-flows.md) |
| 想理解技术栈和设计决策 | [03-technology-notes.md](03-technology-notes.md) |
| 想看简历怎么写 | [04-resume-guide.md](04-resume-guide.md) |
| 想看 54 道面试题 | [05-interview-qa.md](05-interview-qa.md) |
| 想看项目全面评分 | [../docs/项目全面分析报告.md](../docs/项目全面分析报告.md) |
