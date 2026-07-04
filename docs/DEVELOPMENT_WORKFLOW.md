# CodeXX 开发工作流

> 目标：让每次开发都按“确定方向 -> 参考现有框架 -> 开发 -> 验证 -> 文档同步”的固定路径推进，减少上下文遗漏和返工。

---

## 1. 适用范围

本工作流适用于 CodeXX 的所有功能开发、Bug 修复、架构重构、文档维护、打包发布和 Python 能力层演进。

每次进入开发前，先读取：

1. `docs/DEVELOPMENT_WORKFLOW.md`：固定执行流程。
2. `docs/DEVELOPMENT_PLAN.md`：当前进度、优先级和待开发项。
3. 与任务相关的专题文档、学习文档和现有源码。

根目录 `AGENT.md` 是给本地 Agent/Codex 使用的自动化入口，它会指向本工作流。

---

## 2. 文档分工

| 文件或目录 | 作用 |
|------------|------|
| `AGENT.md` | 项目级 Agent 指令，让后续开发默认遵守本工作流 |
| `docs/DEVELOPMENT_WORKFLOW.md` | 稳定工作流，不随单个任务频繁改动 |
| `docs/DEVELOPMENT_PLAN.md` | 实时开发计划，记录优先级、状态、技术债 |
| `docs/49-v19-python-agent-capability-layer-plan.md` | Python 能力层专项计划 |
| `docs/50-modular-ai-assisted-development.md` | 模块化 AI 辅助开发方法论 |
| `docs/架构优化方向.md` | 架构债务和优化方向 |
| `docs/已实现功能与待开发路线.md` | 功能完成度和产品路线 |
| `docs/FileInteractionService-API.md` | 文件交互服务 API 参考 |
| `docs/done/` | 已完成或仅历史参考的文档 |
| `learn/` | 学习、面试、架构理解和接手资料 |

---

## 3. 标准开发流程

### 3.1 阶段一：确定方向

开始写代码前，先把任务压缩成一张任务卡。

任务卡需要包含：

```markdown
## 任务卡

- 任务编号：对应 `docs/DEVELOPMENT_PLAN.md` 中的编号；临时任务使用 `TEMP-日期-序号`
- 任务名称：
- 目标：
- 非目标：
- 影响模块：
- 参考文档：
- 参考源码：
- 验收标准：
- 主要风险：
- 预计验证命令：
```

任务卡应优先写入当前对话或本次开发记录；如果任务会跨会话、跨天或影响路线判断，应同步更新 `docs/DEVELOPMENT_PLAN.md`。

方向确认规则：

- 优先从 `docs/DEVELOPMENT_PLAN.md` 选择 P0 或当前 Phase 项。
- 如果任务不在计划中，先补充到计划或在本次变更说明中明确它为什么优先。
- 如果涉及 Python 能力层，必须确认它只是能力层，不接管 C++ Agent 主循环。
- 如果涉及高权限本机操作，默认允许常规开发命令，但不得主动修改 Windows 系统文件。
- 如果需求边界不清晰，先更新文档和任务卡，再开始编码。

### 3.2 阶段二：参考已有框架

按任务类型读取最小必要上下文，不一次性塞入全项目。

| 任务类型 | 必读文档 | 优先阅读源码 |
|----------|----------|--------------|
| Python 能力层 | `docs/49-v19-python-agent-capability-layer-plan.md`、`learn/07-Python能力层学习.md` | `src/services/PythonSidecar*`、`python/agent_sidecar/`、相关测试 |
| Agent 循环和工具执行 | `docs/已实现功能与待开发路线.md`、`docs/架构优化方向.md` | `src/app/ApplicationController*`、`src/agent/`、`src/tools/` |
| UI 和交互 | `learn/01-architecture.md`、`learn/03-technology-notes.md` | `src/ui/`、`resources/styles/app.qss`、UI 相关测试 |
| 文件能力 | `docs/FileInteractionService-API.md` | `src/services/FileInteractionService*`、文件工具测试 |
| 架构重构 | `docs/50-modular-ai-assisted-development.md`、`docs/架构优化方向.md` | 目标模块和接口调用方 |
| 打包发布 | `README.md`、`docs/done/05-windows-packaging.md` | `CMakeLists.txt`、`release/`、构建脚本 |
| 学习和交接 | `learn/README.md` | 与学习主题对应的源码 |

阅读源码时按这个顺序：

1. 先找接口和调用入口。
2. 再找现有测试。
3. 最后读实现细节。

### 3.3 阶段三：开发

开发规则：

- 保持小步提交思路，每次只改一个明确主题。
- 优先沿用项目现有风格、命名、目录和 Qt 信号槽模式。
- 新增代码注释使用中文，只在复杂逻辑、边界条件和协议约束处添加。
- 新增或更新文档使用中文，面向审查和接手说明。
- 不做无关格式化、不移动无关文件、不重写无关模块。
- 不主动提交、推送或创建发布，除非用户明确要求。
- 保留 C++ 作为主控层：UI、会话、Agent 状态机、工具注册和本机高权限执行仍在 C++。
- Python 只承担能力层：模型调用、多厂商适配、token、embedding、Web、文档解析、浏览器自动化等。

代码修改建议顺序：

1. 先补或调整测试入口。
2. 再改最小实现。
3. 再补 UI、配置或文档。
4. 最后跑验证并处理回归。

### 3.4 阶段四：验证

默认验证命令：

```powershell
git diff --check
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

Python 能力层相关任务额外执行：

```powershell
$env:PYTHONPATH = "D:\C1\CodeXX\python\agent_sidecar"
python -m unittest discover -s python\agent_sidecar\tests
```

UI 相关任务需要补充人工冒烟：

```powershell
.\build\ai_code_assistant.exe
```

重点检查：

- 主窗口能启动。
- 输入、发送、Agent 步骤卡片、复制、搜索等相关路径正常。
- 新增控件不会遮挡、重叠、文字溢出。

打包或发布相关任务额外执行：

```powershell
cmake --build build --config Release -j4
```

如果验证不能执行，最终说明必须写清楚：

- 未执行的命令。
- 原因。
- 当前风险。
- 用户可以如何补跑。

### 3.5 阶段五：实时文档更新

文档更新不是只在开发结束后补一次，而是跟随开发过程实时更新。每次状态、范围或结论发生变化时，先更新对应文档，再继续下一步。

实时更新节点：

- 开始任务时：确认 `docs/DEVELOPMENT_PLAN.md` 中的任务编号、状态和当前 Phase；若不存在，补充临时任务或说明插队原因。
- 方案变化时：如果实现方向、影响模块、验证方式发生变化，立即更新任务卡或相关专题文档。
- 遇到阻塞时：记录阻塞原因、已验证路径和下一步选择，避免新会话重复排查。
- 完成子步骤时：跨会话任务需要更新计划中的状态备注，短任务可在最终收尾时一次更新。
- 验证完成后：记录构建、测试、人工冒烟结果，以及未验证项和剩余风险。
- 任务收尾时：更新完成日期、技术债变化和后续开发建议。

根据影响范围更新文档：

- 修改了计划项：更新 `docs/DEVELOPMENT_PLAN.md` 状态、日期和技术债。
- 完成了专项计划：更新对应专题文档，必要时移入 `docs/done/`。
- 修改了架构边界：更新 `docs/架构优化方向.md` 或新增专题说明。
- 修改了 Python 能力层：更新 `docs/49-v19-python-agent-capability-layer-plan.md` 和 `learn/07-Python能力层学习.md`。
- 修改了学习价值较高的逻辑：更新 `learn/` 中对应学习文档。

完成文档不是形式要求，而是为了让后续 Agent 和人工接手时能直接恢复上下文。

---

## 4. 每次开发的执行清单

```markdown
## 开发执行清单

- [ ] 已读取 `docs/DEVELOPMENT_WORKFLOW.md`
- [ ] 已读取 `docs/DEVELOPMENT_PLAN.md`
- [ ] 已确认任务卡
- [ ] 已确认是否需要实时更新计划或专题文档
- [ ] 已读取任务相关专题文档
- [ ] 已定位入口源码和现有测试
- [ ] 已完成最小范围实现
- [ ] 已补充或调整测试
- [ ] 已更新中文注释或中文文档
- [ ] 已执行 `git diff --check`
- [ ] 已执行构建和测试，或已说明不能执行的原因
- [ ] 已在最终说明中列出变更、验证和剩余风险
```

---

## 5. 分支和合并规则

建议分支命名：

| 类型 | 命名 |
|------|------|
| 功能 | `feature/简短英文主题` |
| Bug 修复 | `fix/简短英文主题` |
| 文档 | `docs/简短英文主题` |
| 重构 | `refactor/简短英文主题` |
| 打包发布 | `release/v版本号` |

合并前检查：

1. `git status --short --branch` 确认改动范围。
2. `git diff --check` 确认没有空白错误。
3. 构建和测试通过。
4. 文档和计划同步。
5. PR 描述写清变更、验证、风险和回滚方式。

处理冲突时：

- 先确认冲突文件属于哪一层。
- 对 `modify/delete` 冲突，先判断删除方是否是文档归档或架构调整，不直接保留任意一边。
- 解决后必须重新运行相关测试。

---

## 6. 完成标准

一项开发只有同时满足以下条件才算完成：

- 功能或修复达到任务卡验收标准。
- 没有引入新的编译错误或测试回归。
- 关键路径有测试或明确的人工冒烟记录。
- 计划文档状态已更新。
- 新增行为、边界或运维要求有中文文档说明。

---

## 7. 禁止事项

- 不在没有任务卡和参考文档的情况下直接大改。
- 不把 Python 改成第二套 Agent 主流程。
- 不让 Python sidecar 直接执行本机高权限文件写入、命令执行、键鼠模拟。
- 不主动修改 Windows 系统文件。
- 不做与当前任务无关的大规模重排、改名或格式化。
- 不删除用户未要求删除的代码、文档、分支或构建产物。
- 不在验证失败时宣称完成。

---

## 8. Agent 自动执行口径

后续 Codex 或项目内 Agent 进入仓库时，默认按以下顺序执行：

1. 读取 `AGENT.md`。
2. 读取本文件。
3. 读取 `docs/DEVELOPMENT_PLAN.md`。
4. 为当前用户请求生成任务卡。
5. 判断是否需要立即更新计划或专题文档。
6. 读取任务相关文档和源码。
7. 完成最小范围开发。
8. 在方向、阻塞、验证结果变化时实时更新文档。
9. 执行验证。
10. 更新计划或学习文档。
11. 最终答复只保留变更、验证和风险。

这个顺序是项目默认工作方式，除非用户在当前对话中明确要求跳过某一步。
