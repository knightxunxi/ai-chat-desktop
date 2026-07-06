# CodeXX 开发计划（实时更新）

> 生成日期：2026-07-05 | 基线：v1.0 + Python sidecar 可切换后端 + 本轮修复闭环完成
> 测试状态：69/69 ✅；Python sidecar 28/28 ✅
> 工作流入口：开始开发前先读取 `AGENT.md` 和 `docs/DEVELOPMENT_WORKFLOW.md`
> 使用说明：每完成一个 Phase 中的项目，请更新对应行状态为 ✅，添加完成日期

---

## 零、执行规则

本文件记录“做什么”和“做到哪了”，具体“怎么做”以 `docs/DEVELOPMENT_WORKFLOW.md` 为准。

每次开发必须按以下顺序推进：

1. 从本文件确认任务编号、优先级和当前 Phase。
2. 按 `docs/DEVELOPMENT_WORKFLOW.md` 填写任务卡。
3. 阅读任务相关专题文档和现有源码。
4. 完成最小范围开发。
5. 执行构建、测试和必要的人工冒烟。
6. 回写本文件的状态、日期和技术债变化。

若任务不在本计划中，先补充计划或在最终说明中解释插队原因。

---

## 一、文档现状检查

### docs/ 中非 done 文件审查结果

| 文件 | 类型 | 状态 |
|------|------|:---:|
| `DOCUMENT_INDEX.md` | 文档总目录 | 🔄 实时维护 |
| `DEVELOPMENT_WORKFLOW.md` | 开发工作流 | 🔄 实时维护 |
| `REVIEW_WORKFLOW.md` | 审查工作流 | 🔄 实时维护 |
| `DEVELOPMENT_PLAN.md` | 开发计划 | 🔄 实时维护 |
| `codebase-memory-mcp-callgraph.md` | MCP 使用方式 + 调用关系分析 | 🔄 随主流程和索引规则实时更新 |
| `52-next-wave-plan.md` | 下一波开发规划 | 🔄 新主规划 |
| `已实现功能.md` | 功能总览 | 🔄 已落地能力全景，完成能力迁入这里 |
| `待开发功能.md` | 功能路线 | 🔄 未完成、占位、暂缓、跳过和弃用功能统一维护 |
| `架构优化方向.md` | 架构优化 | 🔄 8 个方向均未做完 |
| `自更新方向优化.md` | 自更新设计 | 📌 v1.0 仅保留占位入口，真实 GitHub Release 接入后续推进 |
| `游戏助手与YOLO自动化规划.md` | 游戏助手 | ⬚ 主页面入口、YOLO 自动化和受限逆向学习方向规划 |

已完成或被新规划接替的历史专题文档已移入 `docs/done/`：`14-code-style.md`、`49-v19-python-agent-capability-layer-plan.md`、`50-modular-ai-assisted-development.md`、`51-next-four-direction-roadmap.md`、`CODE_REVIEW_DOCUMENT.md`、`FileInteractionService-API.md`。

原 `已实现功能与待开发路线.md` 已按用途拆分为 `docs/已实现功能.md` 和 `docs/待开发功能.md`，不再归档到 `docs/done/`。弃用、跳过和暂缓功能统一放入 `docs/待开发功能.md` 并说明原因。

---

## 二、待开发项全量清单

> 状态标记：⬚ 未开始 | 🔄 进行中 | ✅ 已完成

### V19 Python 能力层（已完成，后续仅做增强）

| # | 开发项 | 状态 | 来源 |
|:-:|--------|:---:|------|
| 1 | Phase A: Python sidecar 骨架 + ping/token.count 协议 | ✅ | V19 Plan |
| 2 | Phase B: C++ PythonSidecarClient QProcess 封装 + 超时 | ✅ | V19 Plan |
| 3 | Phase C: PythonSidecarAIClient 实现 AIClient 接口 + 配置切换 | ✅ 2026-07-04 | V19 Plan |
| 4 | Phase D-1: Python 多厂商适配（OpenAI-compatible） | ✅ 2026-07-04 | V19 Plan |
| 5 | Phase D-2: Python 精确 token 统计 | ✅ 2026-07-04 | V19 Plan |
| 6 | Phase D-3: Web/extract（HTTP + HTML 文本提取） | ✅ 2026-07-04 | V19 Plan |
| 7 | Phase D-4: Document to Markdown 接口 | ✅ 2026-07-04 | V19 Plan |
| 8 | Phase D-5: Playwright 浏览器自动化（下一波 N2） | ⬚ | `52-next-wave-plan.md` |

### P0：高可行性 + 高价值

| # | 开发项 | 状态 | 预估 | 来源 |
|:-:|--------|:---:|:---:|------|
| 9 | **WebSearch 工具** | ✅ 2026-07-04 | `docs/已实现功能.md` |
| 10 | **Token 预算追踪**（每轮统计消耗，边际效益检测） | ✅ 2026-07-04 | `docs/已实现功能.md` |
| 11 | **快捷键系统**（Ctrl+N/K/Enter/Esc/Ctrl+Enter换行） | ✅ 2026-07-04 | `docs/已实现功能.md` |
| 12 | **输入区增强**（↑↓历史 / Ctrl+Enter换行 / @提及文件） | ✅ 2026-07-04 | `docs/已实现功能.md` |
| 13 | **自动打包 CI**（windeployqt + zip + GitHub Release） | ✅ 2026-07-04 | `docs/已实现功能.md` |
| 14 | **使用统计面板**（SQLite 统计 + 纯 UI 展示） | ✅ 2026-07-04 | 1 天 | `docs/已实现功能.md` |
| 15 | **本地模型供应商基础支持**（Ollama via providers） | ✅ 2026-07-04 | 架构优化.md |
| 15b | **本地小模型 PreSend Hook**（ollama/llama.cpp 摘要预处理） | ⬚ | 2-3 天 | 下一波后续增强 |
| 16 | **多厂商适配层 C++ 端**（system.list_providers + sidecar 桥接） | ✅ 2026-07-04 | 架构优化.md |

### P1：中等可行性 + 高价值

| # | 开发项 | 状态 | 预估 | 来源 |
|:-:|--------|:---:|:---:|------|
| 17 | **命令面板 Ctrl+Shift+P**（模糊搜索操作） | ✅ 2026-07-04 | `docs/已实现功能.md` |
| 18 | **斜杠命令**（/clear /export /tools /role /new /help） | ✅ 2026-07-04 | `docs/已实现功能.md` |
| 19 | **AC 继续拆分**（StreamCoordinator 基础） | ✅ 2026-07-04 | 架构优化.md |
| 19b | **MediaCoordinator / ViewManager 拆分** | ⬚ | 2 天 | 下一波后续增强 |
| 20 | **日志脱敏增强**（追加前过 SensitiveFilterHook） | ✅ 2026-07-04 | `docs/已实现功能.md` |
| 21 | **MockApiClient + Agent E2E 循环测试** | ✅ 2026-07-04 | 架构优化.md |

### P2：中等或低可行 / 高工时

| # | 开发项 | 状态 | 预估 | 来源 |
|:-:|--------|:---:|:---:|------|
| 22 | **暗/亮主题颜色变量化**（markdownStyleSheet 已支持双主题） | ✅ 2026-07-04 | 技术债务 |
| 23 | **MCP TLS/认证**（QSslSocket + API Key 透传） | ✅ 2026-07-04 | 技术债务 |
| 24 | **Token 估算精确化**（tiktoken + API usage 解析） | ✅ 2026-07-04 | 技术债务 |
| 25 | **工具并行执行**（QtConcurrent 无依赖并行） | ✅ 2026-07-04 | 架构优化.md |
| 26 | **对话分叉 UI 完善**（分支指示器 + 循环切换） | ✅ 2026-07-04 | `docs/已实现功能.md` |
| 27 | **审查结果结构化**（diff 解析 + 问题分类 + 历史去重） | ✅ 2026-07-04 | `docs/已实现功能.md` |
| 28 | **Catch2 测试框架迁移**（assert → REQUIRE） | ⏭ 跳过 | `docs/待开发功能.md` |
| D-2 | **AgentLoopController 异步化** | ✅ 2026-07-04 | 架构优化.md |

### P3：大工程 / 长期愿景

| # | 开发项 | 状态 | 预估 | 来源 |
|:-:|--------|:---:|:---:|------|
| 29 | **自更新系统**（GitHub Release 检查 + 手动下载） | 📌 2026-07-05 占位 | 2-3 天 | `52-next-wave-plan.md` |
| 30 | **跨平台准备层**（PlatformServices 抽象） | ✅ 2026-07-04 N5 | 3-5 天 | `52-next-wave-plan.md` |
| 31 | **插件系统最小闭环**（QPluginLoader + plugin.json） | ✅ 2026-07-04 N4 | 4-5 天 | `52-next-wave-plan.md` |
| 32 | **角色扮演系统**（Persona + Emotion + Narration） | ⬚ | 5-7 天 | `docs/待开发功能.md` |
| 33 | **双 Agent 互审 + 共享记忆** | ⬚ | 10 天 | 自更新方向.md |
| 34 | **浏览器操作集成**（Playwright via Python sidecar） | ✅ 2026-07-04 N2 | 3-5 天 | `52-next-wave-plan.md` |
| 35 | **游戏自动化 / 游戏助手**（YOLO 识别 + 状态机 + 受控自动化） | ⬚ | 10-15 天 | `docs/游戏助手与YOLO自动化规划.md` |
| 36 | **发布稳定与真实场景验收** | ✅ 2026-07-04 N1 | 1 天 | `52-next-wave-plan.md` |

---

## 三、推荐执行顺序

```
Phase 1 ═══ V19 收尾 ✅（完成）
  ├─ ✅ #3  PythonSidecarAIClient 接入 AIClient
  ├─ ✅ #4  Python 多厂商适配（providers.py + providers.json）
  ├─ ✅ #5  Python 精确 token 统计（tiktoken 可选）
  ├─ ✅ #6  web.extract（URL → HTML → 纯文本）
  ├─ ✅ #7  document.to_markdown（文本+占位符）
  └─ ✅ #16 C++ 端多厂商适配（system.list_providers 工具 + sidecar 桥接）

Phase 2 ═══ 用户能感知的提升（1-2 天）
  ├─ ✅ #11 快捷键系统
  ├─ ✅ #12 输入区增强
  ├─ ✅ #9  WebSearch 工具
  └─ ✅ #10 Token 预算追踪

Phase 3 ═══ 工程化（完成）
  ├─ ✅ #17 命令面板 Ctrl+Shift+P
  ├─ ✅ #18 斜杠命令
  ├─ ✅ #20 日志脱敏增强
  ├─ ✅ #13 自动打包 CI
  └─ ✅ #14 使用统计面板

Phase 4 ═══ 架构深化（完成）
  ├─ ✅ #19 AC 继续拆分（StreamCoordinator 基础）
  ├─ ✅ #21 集成测试框架
  ├─ ✅ #15 本地小模型预处理（Ollama 已在 providers.json 支持）
  └─ ✅ D-3 重复动作检测优化

Phase 5 ═══ 技术债务清理 ✅ 全部完成
  ├─ ✅ #23 MCP TLS/认证
  ├─ ✅ #22 暗/亮主题颜色变量化
  ├─ ✅ #24 Token 估算精确化
  ├─ ✅ #25 工具并行执行
  ├─ ✅ #26 对话分叉 UI 完善
  ├─ ✅ #27 审查结果结构化
  ├─ ⏭ ~~#28 Catch2 测试迁移~~（已评估 — 跳过）
  └─ ✅ D-2 AgentLoopController 异步化

Wave 2 ═══ 52-next-wave-plan（N3 自更新为占位）
  ├─ ✅ N1 发布稳定与真实场景验收
  ├─ ✅ N2 Playwright 浏览器自动化
  ├─ 📌 N3 自更新系统占位
  ├─ ✅ N4 插件系统最小闭环
  └─ ✅ N5 跨平台准备层

Phase 6 ═══ 下一波规划（见 docs/52-next-wave-plan.md）
  ├─ N1 / #36 发布稳定与真实场景验收
  ├─ N2 / #34 Playwright 浏览器自动化
  ├─ N3 / #29 自更新系统
  ├─ N4 / #31 插件系统最小闭环
  ├─ N5 / #30 跨平台准备层
  └─ N6 / #32-#35 长期智能体增强
```

---

## 四、技术债务速查

| 优先级 | 问题 | 文件 | 建议 |
|:---:|------|------|------|
| 🔴 | `markdownStyleSheet()` 硬编码颜色（已支持双主题） | MessageWidget.cpp | ✅ 已通过 isDarkMode() + populateChatView() 解决 |
| 🔴 | Token 估算可用 API usage 精确值 | TokenEstimator.cpp | ✅ 已增加 API usage 解析，可获取实际 token 数 |
| 🟡 | `QEventLoop` 同步等 AI（AiLoopRunner） | AgentLoopController.cpp | 改为全异步信号槽 |
| 🟡 | 重复动作检测基于精确参数匹配（已改为参数键级指纹） | AgentOrchestrator.cpp | ✅ 已优化为参数键级指纹，减少误判 |
| 🟡 | MCP 连接器无 TLS/认证 | McpConnector.cpp | 增加 TLS + API Key 透传 |
| 🟡 | 日志含敏感信息（已增强正则覆盖） | AppLogger.cpp | ✅ 已增加 5 种敏感模式正则脱敏 |
| 🟡 | `release/` 目录手动同步（已集成 CI 自动打包） | 构建脚本 | ✅ 已集成到 GitHub Actions |

---

## 五、下一波规划摘要

详细执行文档见 `docs/52-next-wave-plan.md`。

| 顺序 | 任务包 | 状态 | 并行建议 |
|------|--------|:---:|----------|
| N1 | 发布稳定与真实场景验收 | ⬚ | 可与 N3 设计并行 |
| N2 | Playwright 浏览器自动化 | ⬚ | 可与 N3 并行，不与 Agent 主循环重构并行 |
| N3 | 自更新系统 | 📌 占位 | 可与 N2 并行 |
| N4 | 插件系统最小闭环 | ⬚ | N1 后做，避免打包和加载边界同时不稳 |
| N5 | 跨平台准备层 | ⬚ | 设计可与 N4 并行，代码实现建议串行 |
| N6 | 双 Agent / 角色 / 游戏自动化 | ⬚ | 暂缓 |

---

## 六、文档约定

- 每完成一个 Phase 中的项目，更新状态为 ✅ 并标注完成日期
- 新增开发项的优先级从 P0 到 P3，按可行性 + 价值排序
- 每天开发结束后更新本文件中的进度
- 技术债务解决后从速查表移除

---

## 七、2026-07-04 功能修复记录

| 范围 | 状态 | 说明 |
|------|:---:|------|
| AIClient 后端切换 | ✅ | `ApplicationController` 增加统一信号重连，修复初始化后替换 AIClient 导致请求完成/工具回调断链的问题 |
| Python sidecar 配置 | ✅ | AppConfig/ConfigStorage/SettingsDialog 增加后端类型、Python 命令、sidecar 目录；移除开发机硬编码路径 |
| Sidecar 工具上下文 | ✅ | `system.list_providers` 等工具可从当前 `PythonSidecarAIClient` 获取底层 sidecar client |
| Agent 计划并行执行 | ✅ | 并行工具集合收紧为只读工具，并修复 lambda 引用捕获计划步骤导致的潜在悬空风险 |
| AgentLoopEngine 异步安全 | ✅ | AI 回复文本改为成员变量保存，避免异步信号引用栈变量 |
| 统计面板 | ✅ | 改为通过 `SessionCoordinator`/`ChatHistoryStorage` 读取真实会话统计，不再依赖默认 SQLite 连接 |
| 对话分支 | ✅ | 重新生成时将旧助手回复保存到上一条用户消息分支，并支持切换后持久化 |
| UI 快捷键 | ✅ | 删除重复 `Ctrl+Return` 发送绑定，保留换行行为 |
| MCP TLS | ✅ | 443/8443 TLS 握手失败时不再降级明文连接 |
| 配置保存失败反馈 | ✅ | `ConfigCoordinator::saveConfig` 返回保存结果，失败时保留本次会话配置并给 UI/启动警告 |
| Python sidecar 路径兜底 | ✅ | 配置目录无效时继续检查应用目录、当前目录和默认目录，避免错误持久化路径导致 sidecar 永久不可用 |
| 成功回复重新生成 | ✅ | 新增 `ApplicationController::regenerateLastResponse()`，右键重新生成不再依赖失败重试标记 |
| 子 Agent 客户端隔离 | ✅ | `agent.explore` 使用独立 AIClient，按 direct/sidecar 后端创建，sidecar 不可用时回退直连，避免污染主会话信号 |
| 回归测试补充 | ✅ | `AgentLoopExecutionTest` 增加成功回复重新生成回归用例，测试总数保持 69/69 通过 |

验证：`git diff --check`、`cmake --build build -j4`、`ctest --test-dir build --output-on-failure`、`python -m unittest discover -s python\agent_sidecar\tests` 全部通过。

---

## 八、2026-07-05 审查修复记录

| 范围 | 状态 | 说明 |
|------|:---:|------|
| 浏览器工具业务失败判定 | ✅ | C++ `browser.open` / `browser.extract_text` / `browser.screenshot` 现在会检查 sidecar `result.ok`，不再把 Playwright 缺失或运行失败误判为成功 |
| 浏览器工具输入边界 | ✅ | Python sidecar 仅接受绝对 `http://` / `https://` URL，截图输出目录限制在系统临时目录内 |
| 浏览器资源释放 | ✅ | browser/page/playwright 在异常路径也会关闭，避免 sidecar 残留浏览器进程 |
| 插件发现路径 | ✅ | `AgentOrchestrator` 同时扫描应用目录 `plugins/` 与项目目录 `plugins/`，开发构建产物和发布插件目录均可发现 |
| 插件启停状态 | ✅ | `PluginManager` 跳过重复插件 ID，重新启用插件成功后恢复 `loaded=true` |
| 自动更新 | 📌 | v1.0 保留设置页“检查更新”占位入口，不访问占位 GitHub 仓库；真实 Release 接入后续推进 |
| 发布包依赖 | ✅ | 已补充 `release/AIChatDesktop/Qt6Concurrent.dll`，打包报告已记录 |
| 文档状态 | ✅ | 当时将 `已实现功能与待开发路线.md` 归档到 `docs/done/`；2026-07-06 已按功能拆分策略调整为活跃文档 |

验证：`git diff --check`、`cmake --build build -j4`、`ctest --test-dir build --output-on-failure`、`$env:PYTHONPATH='D:\C1\CodeXX\python\agent_sidecar'; python -m unittest discover -s python\agent_sidecar\tests` 全部通过。

---

## 九、2026-07-06 文档结构调整记录

| 范围 | 状态 | 说明 |
|------|:---:|------|
| 功能总览拆分 | ✅ | 原 `已实现功能与待开发路线.md` 拆分为 `docs/已实现功能.md` 和 `docs/待开发功能.md` |
| 已实现能力维护 | ✅ | 已落地能力统一维护到 `docs/已实现功能.md`，不再和待开发路线混写 |
| 待开发和弃用维护 | ✅ | 未完成、占位、暂缓、跳过和明确不做的功能统一维护到 `docs/待开发功能.md`，并标明原因 |
| done 目录策略 | ✅ | 功能总览类文档不再归档到 `docs/done/`，只归档历史专题、验收记录和旧设计 |

验证：文档结构调整，无需构建；执行 `git diff --check`。
