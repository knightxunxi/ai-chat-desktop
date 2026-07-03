# V19 Python 能力层学习

> 对应分支：`feature/python-agent-capability-layer`  
> 对应规划：`docs/49-v19-python-agent-capability-layer-plan.md`  
> 当前状态：Python sidecar 骨架已建立，Debug 构建通过，CTest 68/68 通过

---

## 一、这次改造解决什么问题

CodeXX 原本是纯 C++/Qt Agent 桌面应用，优点是桌面控制、Win32 能力、UI 和工具执行都很扎实；短板是 AI 能力生态没有 Python 丰富，例如多厂商 SDK、tokenizer、embedding、网页抽取、文档解析、Playwright 浏览器自动化等。

V19 的方向是：

```text
C++ 主程序负责主控：
  UI / 会话 / Agent 循环 / 工具注册 / 权限边界 / 文件与命令执行

Python sidecar 负责能力：
  模型调用 / token 统计 / embedding / WebSearch / 文档解析 / 浏览器自动化
```

关键点：这不是两套 Agent 流程。Agent 的决策循环仍然在 C++，Python 只是能力层。

---

## 二、为什么不用 Python 重写整个 Agent

| 方案 | 优点 | 问题 |
|------|------|------|
| 全部继续 C++ | 单一语言、部署简单 | AI 生态扩展慢，接 tokenizer/embedding/Web/Playwright 成本高 |
| Python 重写 Agent | AI 库丰富 | 会丢掉现有 Qt UI、Win32 工具、安全边界和 68 项测试积累 |
| C++ 主控 + Python sidecar | 保留桌面工程优势，同时接入 Python AI 生态 | 需要设计进程协议、超时、错误处理和打包策略 |

当前选择第三种，因为它最符合项目现状：C++ 已经有完整 Agent 平台，Python 只补能力短板。

---

## 三、当前已经落地的文件

### Python 侧

| 文件 | 作用 |
|------|------|
| `python/agent_sidecar/agent_sidecar/server.py` | JSONL 服务入口，从 stdin 读请求，从 stdout 写响应 |
| `python/agent_sidecar/agent_sidecar/protocol.py` | 解析请求、分发 method、包装成功/失败响应 |
| `python/agent_sidecar/agent_sidecar/capabilities.py` | 能力实现，当前包含 `ping`、`token.count`、`model.chat` mock/非流式调用 |
| `python/agent_sidecar/tests/test_protocol.py` | Python 单元测试，覆盖 ping、token 统计、错误响应和子进程 smoke |

### C++ 侧

| 文件 | 作用 |
|------|------|
| `src/services/PythonSidecarProtocol.h/.cpp` | 构造 JSONL 请求，解析 sidecar 响应 |
| `src/services/PythonSidecarClient.h/.cpp` | 用 `QProcess` 启动 Python sidecar，发送请求，处理超时和 stderr |
| `tests/services/PythonSidecarProtocolTest.cpp` | 测 C++ 协议构造/解析 |
| `tests/services/PythonSidecarClientTest.cpp` | 测 C++ 能否真实启动 Python sidecar 并完成请求 |

---

## 四、协议怎么设计

传输方式：一行一个 JSON 对象，stdin/stdout 双向通信。

请求：

```json
{"id":"cpp-1","method":"token.count","params":{"text":"hello 世界"}}
```

成功响应：

```json
{"id":"cpp-1","ok":true,"result":{"tokens":4,"chars":8,"method":"estimated-cjk-aware-v1"}}
```

失败响应：

```json
{"id":"cpp-1","ok":false,"error":{"code":"method_not_found","message":"Unsupported sidecar method: xxx.","retryable":false}}
```

为什么要有 `id`：

- C++ 可以确认响应属于哪次请求。
- 以后支持并发请求或流式事件时，不会混淆响应。

为什么要有 `ok/error/retryable`：

- C++ 不需要猜测异常类型。
- UI 后续可以把错误展示成更明确的用户提示。
- Agent 可以区分“可重试错误”和“参数错误”。

---

## 五、审查重点

审查 Python sidecar 时重点看：

- 输入是否校验：`id`、`method`、`params` 必须有明确类型。
- 错误是否结构化：异常不能直接让进程崩溃。
- 是否避免接管权限：Python 不执行本机文件写入、命令执行、鼠标键盘模拟。
- 依赖是否克制：当前只用标准库，先把协议跑通。

审查 C++ client 时重点看：

- `QProcess` 是否设置超时。
- sidecar 退出时是否返回明确错误。
- stdout/stderr 是否分开处理。
- 协议解析失败是否不会让 Agent 静默完成。

---

## 六、后续学习路线

1. 先读 `PythonSidecarProtocol`，理解 JSONL 的最小协议。
2. 再读 `PythonSidecarClient`，理解 `QProcess` 如何管理子进程。
3. 再读 Python 的 `protocol.py`，理解请求校验和结构化错误。
4. 最后读 `capabilities.py`，理解每个能力如何保持可替换。

后续可继续扩展：

- `model.chat` 接入真实 OpenAI-compatible 非流式调用。
- `token.count` 替换为 `tiktoken` 或 Hugging Face tokenizer。
- `web.extract` 接入网页正文抽取。
- `document.to_markdown` 接入 PDF/Office 文档解析。
- `browser.run` 接入 Playwright，但仍由 C++ Agent 决定何时调用。

---

## 七、面试表达

可以这样讲：

> 我没有把原来的 C++ Agent 重写成 Python，而是做了一个 Python sidecar 能力层。C++ 继续负责 Qt UI、Agent 主循环、工具权限和本机操作，Python 通过 JSONL 协议提供模型调用、token 统计、网页解析、文档解析等能力。这样既保留了 C++ 桌面工程优势，也能接入 Python AI 生态。

如果被问“为什么不用 LangChain”：

> 因为项目已经有自己的 Agent 循环、工具注册、记忆、Hooks 和安全边界。现在引入 LangChain 会和已有架构重叠。更合理的是先把 Python 作为能力层，等有明确需求时再选择具体库，而不是让外部框架接管主循环。
