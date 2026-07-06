# CodeXX 文档总目录

> 创建日期：2026-07-04
> 维护规则：新增、归档、完成或废弃文档时必须更新本目录。
> 状态标记：🔄 进行中 | ✅ 已完成 | 📌 参考资料 | ⬚ 计划中 | ❌ 未实施 | 🗄️ 已归档

---

## 1. 使用方式

后续 Agent 或人工接手时，先读本目录，再按任务方向读取具体文档。

推荐顺序：

1. `AGENT.md`
2. `docs/DOCUMENT_INDEX.md`
3. `docs/DEVELOPMENT_WORKFLOW.md`
4. `docs/DEVELOPMENT_PLAN.md`
5. `docs/codebase-memory-mcp-callgraph.md`
6. 任务相关专题文档

不要一次性读取所有文档。先用本目录判断方向，再用 `codebase-memory-mcp` 查询相关类、方法和调用关系，最后只读取必要文档和源码。

---

## 2. 当前活跃文档

| 文档 | 方向 | 状态 | 日期 | 简要说明 |
|------|------|:---:|------|----------|
| `AGENT.md` | Agent 入口 | 🔄 | 2026-07-04 | 项目级默认指令，规定后续 Agent 先读哪些工作流和文档 |
| `docs/DOCUMENT_INDEX.md` | 文档管理 | 🔄 | 2026-07-04 | docs 总目录，记录方向、状态和维护规则 |
| `docs/DEVELOPMENT_WORKFLOW.md` | 开发流程 | 🔄 | 2026-07-04 | 标准开发流程，包含 MCP token 节省、实时文档更新和验证规则 |
| `docs/REVIEW_WORKFLOW.md` | 审查流程 | 🔄 | 2026-07-04 | 企业级审查流程，包含影响分析、问题等级、验证和学习沉淀 |
| `docs/DEVELOPMENT_PLAN.md` | 开发计划 | 🔄 | 2026-07-04 | 实时计划、优先级、技术债和后续路线 |
| `docs/52-next-wave-plan.md` | 下一波规划 | 🔄 | 2026-07-04 | 修复闭环后的下一阶段任务、并行关系和验收标准 |
| `docs/已实现功能.md` | 功能总览 | 🔄 | 2026-07-06 | 当前已经落地的功能全景和验收基线 |
| `docs/待开发功能.md` | 功能路线 | 🔄 | 2026-07-06 | 未完成、占位、暂缓、跳过和弃用功能清单 |
| `docs/codebase-memory-mcp-callgraph.md` | 调用关系 | 🔄 | 2026-07-04 | MCP 使用方式、调用关系分析、开发前后索引规则 |
| `docs/架构优化方向.md` | 架构优化 | 🔄 | 2026-06-18 | 架构债务、拆分方向和长期优化 |
| `docs/自更新方向优化.md` | 自更新 | 📌 | 2026-07-05 | 自更新系统设计；v1.0 仅保留检查更新占位入口，真实 Release 接入后续推进 |
| `docs/游戏助手与YOLO自动化规划.md` | 游戏助手 | ⬚ | 2026-07-06 | 主页面游戏助手入口、YOLO 自动化助手和受限逆向学习助手规划 |

---

## 3. 参考文档

| 文档 | 方向 | 状态 | 日期 | 简要说明 |
|------|------|:---:|------|----------|
| `docs/done/14-code-style.md` | 代码规范 | 🗄️ | 2026-05-27 | 已实施的基础代码规范 |
| `docs/done/49-v19-python-agent-capability-layer-plan.md` | Python 能力层 | 🗄️ | 2026-07-04 | Python sidecar 专项计划，全部 Phase 已完成 |
| `docs/done/50-modular-ai-assisted-development.md` | AI 辅助开发方法论 | 🗄️ | 2026-06-21 | 模块化 AI 开发方法、上下文控制和回归验证思路 |
| `docs/v1.0-acceptance-checklist.md` | 验收 | ✅ | 2026-07-04 | v1.0 手工验收清单，7 大模块 40+ 验收项 |
| `docs/v1.0-agent-scenarios.md` | 验收 | ✅ | 2026-07-04 | 8 个真实 Agent 场景脚本 |
| `docs/v1.0-packaging-report.md` | 发布 | ✅ | 2026-07-04 | Release 打包检查报告 |
| `docs/done/51-next-four-direction-roadmap.md` | 四方向规划 | 🗄️ | 2026-07-04 | S1-S4 已完成，后续由 `52-next-wave-plan.md` 接替 |
| `docs/done/CODE_REVIEW_DOCUMENT.md` | 代码审查 | 🗄️ | 2026-07-04 | 功能↔代码位置映射归档，60+ 工具定位，含设计决策和测试结构 |
| `docs/done/FileInteractionService-API.md` | API 参考 | 🗄️ | 2026-06-05 | 文件交互服务 API 说明 |

---

## 4. 已归档文档

`docs/done/` 存放历史版本、验收记录、旧设计和已完成专题。归档文档默认不作为当前开发入口，只有排查历史决策、打包、版本回顾时读取。

### 4.1 版本路线和验收

| 范围 | 状态 | 简要说明 |
|------|:---:|----------|
| `01-requirements.md` 到 `13-v4-roadmap.md` | 🗄️ | V1-V4 需求、设计、任务拆解、验收和发布记录 |
| `16-v5-roadmap.md` 到 `21-v6-acceptance-notes.md` | 🗄️ | V5/V6 会话组织、本地交互、安全和验收 |
| `22-agent-automation-roadmap.md` 到 `30-v8-2-v8-3-acceptance-notes.md` | 🗄️ | Agent 自动化、工作区、V7/V8 验收 |
| `31-v9-plus-development-roadmap.md` 到 `38-v10-3-project-memory-acceptance-notes.md` | 🗄️ | 命令执行、Function Calling、项目指令、外部技能和记忆 |
| `39-v11-plus-development-roadmap.md` 到 `48-v17-remaining-features-plan.md` | 🗄️ | 工具生态、感知能力、Skills/Hooks、AI 管家、体验增强 |
| `V18_CHANGELOG.md` | 🗄️ | V18 变更记录 |

### 4.2 历史设计和分析

| 文档 | 状态 | 简要说明 |
|------|:---:|----------|
| `docs/done/system_design.md` | 🗄️ | 旧版系统设计和模块拆分方案 |
| `docs/done/class-diagram.mermaid` | 🗄️ | 历史类图 |
| `docs/done/sequence-diagram.mermaid` | 🗄️ | 历史时序图 |
| `docs/done/项目全面分析报告.md` | 🗄️ | 项目全面分析旧报告 |
| `docs/done/优化方向.md` | 🗄️ | 旧优化方向汇总 |
| `docs/done/重构方案.md` | 🗄️ | 旧重构方案 |
| `docs/done/UI优化方向.md` | 🗄️ | 旧 UI 优化方向 |
| `docs/done/ai coding thought/` | 🗄️ | AI 编程思考资料，包含人读版、轻量模型版、审查模型版 |

---

## 5. 按方向查文档

| 方向 | 优先阅读 |
|------|----------|
| 开发任务 | `DEVELOPMENT_WORKFLOW.md`、`DEVELOPMENT_PLAN.md`、`codebase-memory-mcp-callgraph.md` |
| 审查任务 | `REVIEW_WORKFLOW.md`、`codebase-memory-mcp-callgraph.md`、相关 diff 和测试 |
| Python 能力层 | `learn/07-Python能力层学习.md`、`docs/done/49-v19-python-agent-capability-layer-plan.md` |
| 功能总览 | `已实现功能.md`、`待开发功能.md`、`DEVELOPMENT_PLAN.md` |
| 下一波规划 | `待开发功能.md`、`52-next-wave-plan.md`、`DEVELOPMENT_PLAN.md` |
| Agent 循环 | `codebase-memory-mcp-callgraph.md`、`DEVELOPMENT_PLAN.md` |
| 架构优化 | `架构优化方向.md`、`docs/done/50-modular-ai-assisted-development.md` |
| 工具系统 | `docs/done/FileInteractionService-API.md`、`docs/done/15-tool-integration-design.md` |
| 游戏助手 | `docs/游戏助手与YOLO自动化规划.md`、`DEVELOPMENT_PLAN.md` 中 #35 |
| 打包发布 | `README.md`、`docs/done/05-windows-packaging.md` |
| 学习交接 | `learn/README.md`、`learn/01-architecture.md`、`learn/02-key-flows.md` |

---

## 6. 维护规则

新增文档时：

1. 放入 `docs/` 或合适的子目录。
2. 在本目录登记方向、状态、日期和说明。
3. 如果是开发流程、审查流程或调用关系文档，同步更新 `AGENT.md`。

完成文档时：

1. 将状态改为 ✅ 或 🗄️。
2. 如果不再作为当前入口，移入 `docs/done/`；功能总览类文档例外，优先拆分维护在 `docs/已实现功能.md` 和 `docs/待开发功能.md`。
3. 更新 `docs/DEVELOPMENT_PLAN.md` 的文档现状检查。

废弃文档时：

1. 不直接删除，优先移入 `docs/done/`；废弃功能或跳过功能记录到 `docs/待开发功能.md` 并说明原因。
2. 在本目录说明废弃原因或替代文档。
3. 若包含错误结论，在文件顶部添加废弃说明。

每次开发完成后：

- 若新增、移动、归档或修改关键文档，必须更新本目录。
- 若调用关系或主流程变化，必须更新 `docs/codebase-memory-mcp-callgraph.md`。
- 若计划状态变化，必须更新 `docs/DEVELOPMENT_PLAN.md`。
