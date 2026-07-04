# CodeXX 审查工作流

> 创建日期：2026-07-04
> 适用范围：代码审查、架构审查、文档审查、发布前审查、Agent 生成改动复查
> 目标：按企业级项目标准发现缺陷、控制风险、沉淀学习记录，而不是只做格式化检查。

---

## 1. 审查原则

审查优先级从高到低：

1. 正确性：是否会导致功能错误、数据丢失、流程中断。
2. 安全性：是否扩大权限、泄露密钥、绕过边界、误改系统文件。
3. 回归风险：是否影响主流程、工具执行、存储、配置、UI 关键交互。
4. 可维护性：是否破坏模块边界、引入重复逻辑、增加隐式耦合。
5. 可验证性：是否有测试、构建、人工冒烟或明确的未验证风险。
6. 文档一致性：计划、调用关系、学习文档是否同步。

审查不是重写代码。除非用户明确要求“修复”，否则审查只输出问题、证据、风险和建议。

---

## 2. 审查输入

每次审查前收集以下输入：

| 输入 | 获取方式 |
|------|----------|
| 当前分支和改动范围 | `git status --short --branch`、`git diff --stat` |
| 具体 diff | `git diff -- <path>` |
| MCP 影响范围 | `codebase-memory-mcp detect_changes` |
| 相关调用关系 | `search_graph`、`trace_path`、`query_graph` |
| 相关源码 | 只读被影响文件和直接调用方 |
| 相关测试 | 目标模块测试 + 必要集成测试 |
| 相关文档 | `docs/DOCUMENT_INDEX.md` 中对应方向的文档 |

默认 MCP 命令：

```powershell
D:\MCP\codebase-memory-mcp\codebase-memory-mcp.exe cli index_status '{"project":"D-C1-CodeXX"}'
D:\MCP\codebase-memory-mcp\codebase-memory-mcp.exe cli detect_changes '{"project":"D-C1-CodeXX"}'
```

如果 `detect_changes` 结果不包含未跟踪文件，审查时必须额外查看：

```powershell
git ls-files --others --exclude-standard
```

---

## 3. 审查流程

### 3.1 阶段一：确认范围

执行：

```powershell
git status --short --branch
git diff --stat
```

确认：

- 是否有用户未提交或无关改动。
- 是否包含未跟踪文件。
- 是否存在生成文件、二进制、密钥、缓存。
- 是否和当前任务目标一致。

### 3.2 阶段二：MCP 影响分析

执行：

```powershell
D:\MCP\codebase-memory-mcp\codebase-memory-mcp.exe cli detect_changes '{"project":"D-C1-CodeXX"}'
```

根据结果继续查：

```powershell
D:\MCP\codebase-memory-mcp\codebase-memory-mcp.exe cli search_graph '{"project":"D-C1-CodeXX","name_pattern":".*目标名称.*","limit":20}'
D:\MCP\codebase-memory-mcp\codebase-memory-mcp.exe cli trace_path '{"project":"D-C1-CodeXX","function_name":"目标函数","direction":"both","depth":2}'
```

注意：Qt/C++ 成员函数的 callee 识别可能不完整，MCP 结果必须和源码核对。

### 3.3 阶段三：分层审查

按影响层级检查：

| 层级 | 审查重点 |
|------|----------|
| UI | 信号槽连接、状态同步、文字溢出、控件遮挡、用户操作闭环 |
| App | `ApplicationController`、Agent 状态、分支路径、上下文恢复 |
| Core | 数据模型、枚举、序列化兼容性 |
| Service | AI 请求、流式解析、sidecar 超时、错误分类 |
| Storage | SQLite schema、迁移、读写一致性、路径安全 |
| Support | 日志、脱敏、诊断信息 |
| Tools | 权限边界、参数校验、失败返回、Hook 执行 |
| Agent | 循环终止、重复动作、工具观察、token 预算 |
| Python | 协议兼容、异常结构、依赖边界、不可接管主流程 |
| MCP | 外部工具注册、连接状态、错误隔离 |
| Scheduler | cron 解析、任务持久化、重复触发 |

### 3.4 阶段四：验证审查

常规验证：

```powershell
git diff --check
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

Python sidecar 变更：

```powershell
$env:PYTHONPATH = "D:\C1\CodeXX\python\agent_sidecar"
python -m unittest discover -s python\agent_sidecar\tests
```

UI 变更：

```powershell
.\build\ai_code_assistant.exe
```

不能执行验证时，审查结论必须降级为“有未验证风险”，不能写“通过”。

### 3.5 阶段五：文档一致性审查

检查：

- `docs/DEVELOPMENT_PLAN.md` 是否更新状态、日期、技术债。
- `docs/codebase-memory-mcp-callgraph.md` 是否反映新的主流程或调用边界。
- `docs/DOCUMENT_INDEX.md` 是否新增、归档或更新文档状态。
- Python 能力层变更是否更新 `docs/49-v19-python-agent-capability-layer-plan.md`。
- 学习价值较高的变更是否更新 `learn/`。

---

## 4. 问题等级

| 等级 | 定义 | 处理规则 |
|------|------|----------|
| P0 阻断 | 会导致构建失败、数据丢失、安全越界、主流程不可用 | 必须修复后才能合并 |
| P1 高风险 | 关键路径回归、测试缺失、状态机错误、权限边界模糊 | 原则上修复后合并 |
| P2 中风险 | 可维护性下降、边界不清、错误处理不足 | 可带记录合并，但需进入计划 |
| P3 建议 | 命名、注释、轻微重复、体验细节 | 可后续优化 |

审查输出必须按 P0 -> P3 排序，先列问题，再列总结。

---

## 5. 审查输出模板

```markdown
## 审查结论

- 结论：通过 / 有条件通过 / 不通过
- 范围：
- MCP 影响分析：
- 已执行验证：
- 未验证风险：

## 问题

1. [P级别] 标题
   - 文件：
   - 证据：
   - 风险：
   - 建议：

## 文档同步

- 已更新：
- 需要补充：

## 学习记录

- 本次暴露的问题类型：
- 后续避免方式：
```

---

## 6. 企业级学习要求

作为学习项目，每次审查尽量沉淀一条可复用经验：

- 缺陷属于需求、设计、实现、测试、文档还是发布问题。
- 哪个流程节点本应提前发现它。
- 下次应增加哪条检查项、测试或文档说明。

如果审查发现重复问题，应回写到：

- `docs/DEVELOPMENT_WORKFLOW.md`：流程缺口。
- `docs/DEVELOPMENT_PLAN.md`：待开发或技术债。
- `learn/`：学习材料。
- `docs/DOCUMENT_INDEX.md`：文档状态和维护记录。

---

## 7. 审查完成标准

一次审查完成需要满足：

- 已说明审查范围。
- 已基于 MCP 或 diff 给出影响分析。
- 已列出 P0/P1/P2/P3 问题或明确无问题。
- 已说明验证命令和结果。
- 已说明未验证风险。
- 已检查文档同步。
- 审查结论可被另一个 Agent 复核。
