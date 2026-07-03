# V19 Python Agent Capability Layer Plan

> Date: 2026-07-03  
> Branches: `baseline/cpp-agent-v1.0`, `feature/python-agent-capability-layer`  
> Baseline: Qt6/C++ desktop Agent platform, v1.0, current C++ Agent loop preserved

---

## 1. Goal

V19 的目标不是重写桌面端，也不是引入第二套 Agent 主流程，而是在现有 C++ Agent 主循环之下增加一个可替换的 Python 能力层：

```text
Qt/C++ 主程序
  UI / 会话 / Agent 状态机 / 工具注册 / 权限边界 / 本机文件与命令执行
        |
        | JSONL or JSON-RPC over QProcess
        v
Python capability sidecar
  模型调用 / 多厂商适配 / token 统计 / embedding / WebSearch / 文档解析 / 浏览器自动化
```

核心约束：

- 保留 C++ 作为主控层，`ApplicationController`、`AgentOrchestrator`、`AgentToolRegistry` 不迁移到 Python。
- Python 只提供能力，不直接执行本机高权限文件修改、命令执行或桌面操作。
- Agent 循环仍然只有一条：分析 -> 工具执行 -> 观测 -> 继续。
- 默认权限保持项目当前策略：Agent 可执行常规开发任务，但不主动修改 Windows 系统文件。
- DeepSeek 个人 API 暂不开放图片能力，图片通道保留为后续多模态备用能力。

---

## 2. Branch Strategy

| Branch | Purpose | Rule |
|--------|---------|------|
| `baseline/cpp-agent-v1.0` | 保留当前 C++ v1.0 主线基线 | 不做 Python sidecar 改造，只接受必要文档或热修复 |
| `feature/python-agent-capability-layer` | 开发 Python Agent 能力层 | 所有 sidecar、后端抽象、协议和测试先进入此分支 |

当前策略：

1. C++ 基线分支用于回滚、对比和继续纯 C++ UI/逻辑优化。
2. Python 分支先做可并存后端，默认仍可走现有 `OpenAICompatibleClient`。
3. Python 能力层稳定后，再决定是否合并回主线。

---

## 3. V19 Scope

### 3.1 Must Have

| Item | Owner Layer | Deliverable |
|------|-------------|-------------|
| AI backend abstraction | C++ | `AIBackend`/`AIClient` 适配边界，保留现有 OpenAI-compatible 客户端 |
| Python sidecar protocol | C++ + Python | QProcess JSONL 请求/响应协议，带 request id、错误码和超时 |
| Model completion capability | Python | `model.chat` 命令，兼容 OpenAI-compatible API |
| Token count capability | Python | `token.count` 命令，先用轻量估算，后续替换为精确 tokenizer |
| Config switch | C++ | 可通过配置选择 direct C++ backend 或 Python sidecar backend |
| Tests | C++ + Python | C++ 协议构造测试 + Python sidecar 单元测试 |

### 3.2 Should Have

| Item | Owner Layer | Deliverable |
|------|-------------|-------------|
| Multi-provider adapter | Python | OpenAI-compatible first, LiteLLM optional later |
| Web extraction | Python | `web.extract` using HTTP + HTML text extraction |
| Document to Markdown | Python | `document.to_markdown` placeholder interface |
| Structured logs | Both | sidecar lifecycle, request latency, error category |

### 3.3 Out of Scope for First Cut

- Python 接管 Agent 决策循环。
- Python 执行 `file.write`、`command.exec`、鼠标键盘模拟等高权限本机工具。
- 一次性接入 LangChain/LlamaIndex 等完整 Agent 框架。
- 将现有 C++ 工具注册表迁移到 Python。

---

## 4. Protocol Draft

Transport: one JSON object per line over `stdin/stdout`.

Request:

```json
{"id":"req-1","method":"model.chat","params":{"provider":"openai-compatible","base_url":"https://api.deepseek.com","model":"deepseek-v4-flash","messages":[],"tools":[],"stream":false}}
```

Success response:

```json
{"id":"req-1","ok":true,"result":{"text":"...","tool_calls":[],"usage":{"prompt_tokens":0,"completion_tokens":0}}}
```

Error response:

```json
{"id":"req-1","ok":false,"error":{"code":"provider_error","message":"...","retryable":true}}
```

Streaming can be added later with event frames:

```json
{"id":"req-1","event":"delta","delta":"..."}
{"id":"req-1","event":"done","usage":{"prompt_tokens":0,"completion_tokens":0}}
```

---

## 5. Development Phases

### Phase A: Planning and skeleton

- Status: done on `feature/python-agent-capability-layer`.
- Added this plan.
- Added `python/agent_sidecar` with a minimal JSONL server.
- Added smoke tests for `ping`, `token.count`, `model.chat` mock response, invalid JSON, and invalid method handling.
- Runtime UI is not wired to Python yet.

### Phase B: C++ protocol client

- Status: started on `feature/python-agent-capability-layer`.
- Added `PythonSidecarProtocol` for JSONL request construction and response parsing.
- Added `PythonSidecarClient` around `QProcess`.
- Added request timeout, stderr capture, process stop, and JSON parse error handling.
- Kept it isolated from `OpenAICompatibleClient` until protocol tests are stable.

### Phase C: Backend switch

- Add `PythonSidecarAIClient` implementing `AIClient`.
- Keep direct C++ HTTP as default.
- Add config flag for experimental Python backend.

### Phase D: Capability growth

- Add real OpenAI-compatible Python call.
- Add precise token counting.
- Add web extraction and document parsing.
- Add browser automation through Playwright only after protocol and lifecycle are stable.

---

## 6. Acceptance Criteria

V19 can be considered ready when:

1. Existing C++ build and CTest remain green.
2. Python sidecar tests pass locally.
3. C++ direct AI backend still works without Python installed.
4. Python backend can be enabled without changing Agent loop code.
5. Sidecar failure degrades into a clear user-visible error, not a silent Agent completion.
6. Tool execution remains owned by C++ and still follows current permission boundaries.
