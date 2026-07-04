# CodeXX 开发计划（实时更新）

> 生成日期：2026-07-04 | 基线：v1.0 + Python Sidecar 骨架已完成  
> 测试状态：68/68 ✅  
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
| `14-code-style.md` | 代码规范（流程文档） | ✅ 已实施 |
| `DOCUMENT_INDEX.md` | 文档总目录 | 🔄 实时维护 |
| `REVIEW_WORKFLOW.md` | 审查工作流 | 🔄 实时维护 |
| `codebase-memory-mcp-callgraph.md` | MCP 使用方式 + 调用关系分析 | 🔄 随主流程和索引规则实时更新 |
| `50-modular-ai-assisted-development.md` | 方法论文档 | ✅ 仅文档 |
| `FileInteractionService-API.md` | API 参考文档 | ✅ 仅文档 |
| `49-v19-python-agent-capability-layer-plan.md` | V19 计划 | 🔄 Phase A+B 完成，C+D 待做 |
| `已实现功能与待开发路线.md` | 功能清单 + 路线 | 🔄 含多个待实施项 |
| `架构优化方向.md` | 架构优化 | 🔄 8 个方向均未做完 |
| `自更新方向优化.md` | 自更新设计 | ❌ 未实施 |

---

## 二、待开发项全量清单

> 状态标记：⬚ 未开始 | 🔄 进行中 | ✅ 已完成

### V19 Python 能力层（当前进行中）

| # | 开发项 | 状态 | 来源 |
|:-:|--------|:---:|------|
| 1 | Phase A: Python sidecar 骨架 + ping/token.count 协议 | ✅ | V19 Plan |
| 2 | Phase B: C++ PythonSidecarClient QProcess 封装 + 超时 | ✅ | V19 Plan |
| 3 | Phase C: PythonSidecarAIClient 实现 AIClient 接口 + 配置切换 | ⬚ | V19 Plan |
| 4 | Phase D-1: Python 多厂商适配（OpenAI-compatible） | ⬚ | V19 Plan |
| 5 | Phase D-2: Python 精确 token 统计 | ⬚ | V19 Plan |
| 6 | Phase D-3: Web/extract（HTTP + HTML 文本提取） | ⬚ | V19 Plan |
| 7 | Phase D-4: Document to Markdown 接口 | ⬚ | V19 Plan |
| 8 | Phase D-5: Playwright 浏览器自动化（后期） | ⬚ | V19 Plan |

### P0：高可行性 + 高价值

| # | 开发项 | 状态 | 预估 | 来源 |
|:-:|--------|:---:|:---:|------|
| 9 | **WebSearch 工具** | ⬚ | 1 天 | 已实现路线.md |
| 10 | **Token 预算追踪**（每轮统计消耗，边际效益检测） | ⬚ | 1 天 | 已实现路线.md |
| 11 | **快捷键系统**（Ctrl+N/K/L/Enter/Esc） | ⬚ | 0.5 天 | 已实现路线.md |
| 12 | **输入区增强**（↑↓历史 / Ctrl+Enter换行 / @提及文件） | ⬚ | 1 天 | 已实现路线.md |
| 13 | **自动打包 CI**（windeployqt + zip + GitHub Release） | ⬚ | 1 天 | 已实现路线.md |
| 14 | **使用统计面板**（SQLite 统计 + 纯 UI 展示） | ⬚ | 1 天 | 已实现路线.md |
| 15 | **本地小模型预处理**（ollama/llama.cpp Hook） | ⬚ | 3 天 | 架构优化.md |
| 16 | **多厂商适配层 C++ 端**（ModelAdapter 接口） | ⬚ | 2 天 | 架构优化.md |

### P1：中等可行性 + 高价值

| # | 开发项 | 状态 | 预估 | 来源 |
|:-:|--------|:---:|:---:|------|
| 17 | **命令面板 Ctrl+K**（模糊搜索会话/工具/操作） | ⬚ | 1 天 | 已实现路线.md |
| 18 | **斜杠命令**（/clear /export /tools /role） | ⬚ | 0.5 天 | 已实现路线.md |
| 19 | **AC 继续拆分**（StreamCoordinator + MediaCoordinator） | ⬚ | 2 天 | 架构优化.md |
| 20 | **日志脱敏增强**（追加前过 SensitiveFilterHook） | ⬚ | 0.5 天 | 已实现路线.md |
| 21 | **集成测试框架**（MockApiClient + E2E Agent 循环） | ⬚ | 3 天 | 架构优化.md |

### P2：中等或低可行 / 高工时

| # | 开发项 | 状态 | 预估 | 来源 |
|:-:|--------|:---:|:---:|------|
| 22 | **暗/亮主题颜色变量化**（markdownStyleSheet 改造） | ⬚ | 1 天 | 技术债务 |
| 23 | **MCP TLS/认证**（QSslSocket + API Key 透传） | ⬚ | 2 天 | 技术债务 |
| 24 | **Token 估算精确化**（tiktoken 集成） | ⬚ | 2 天 | 技术债务 |
| 25 | **工具并行执行**（QtConcurrent 无依赖并行） | ⬚ | 4 天 | 架构优化.md |
| 26 | **对话分叉 UI 完善**（消息树可视化） | ⬚ | 2 天 | 已实现路线.md |
| 27 | **CR 平台化**（GitHub 多人参与 CR 流） | ⬚ | 3 天 | 模块化方向.md |
| 28 | **Catch2 测试框架迁移**（assert → REQUIRE） | ⬚ | 3 天 | 架构优化.md |

### P3：大工程 / 长期愿景

| # | 开发项 | 状态 | 预估 | 来源 |
|:-:|--------|:---:|:---:|------|
| 29 | **自更新系统**（GitHub API + 增量下载） | ⬚ | 3 天 | 已实现路线.md |
| 30 | **跨平台 macOS/Linux** | ⬚ | 5-10 天 | 架构优化.md |
| 31 | **插件系统**（QPluginLoader + plugin.json） | ⬚ | 5 天 | 架构优化.md |
| 32 | **角色扮演系统**（Persona + Emotion + Narration） | ⬚ | 5-7 天 | 已实现路线.md |
| 33 | **双 Agent 互审 + 共享记忆** | ⬚ | 10 天 | 自更新方向.md |
| 34 | **浏览器操作集成**（Playwright） | ⬚ | 5 天 | 已实现路线.md |
| 35 | **游戏自动化**（状态机 + 模板匹配） | ⬚ | 10-15 天 | 已实现路线.md |

---

## 三、推荐执行顺序

```
Phase 1 ═══ V19 收尾（当前进行中，1-2 天）
  ├─ #3  PythonSidecarAIClient 接入 AIClient     ← 先让 sidecar 全链路跑通
  ├─ #4  Python 多厂商适配                         
  └─ #5  Python 精确 token 统计                    

Phase 2 ═══ 用户能感知的提升（1-2 天）
  ├─ #11 快捷键系统                              ← 零架构改动，体验质变
  ├─ #12 输入区增强（历史/换行/@提及）              
  ├─ #9  WebSearch 工具                           ← 1 天完成，Agent 新能力
  └─ #10 Token 预算追踪                           ← 1 天纯逻辑

Phase 3 ═══ 工程化（2-3 天）
  ├─ #17 命令面板 Ctrl+K                        
  ├─ #18 斜杠命令                                 
  ├─ #20 日志脱敏增强                             
  ├─ #13 自动打包 CI（windeployqt + GitHub Release）
  └─ #14 使用统计面板                             

Phase 4 ═══ 架构深化（4-5 天）
  ├─ #19 AC 继续拆分（StreamCoordinator + MediaCoordinator）
  ├─ #21 集成测试框架（MockApiClient + E2E Agent 循环）
  └─ #15 本地小模型预处理                         

Phase 5 ═══ 技术债务清理（3-5 天）
  ├─ #23 MCP TLS/认证
  ├─ #22 暗/亮主题颜色变量化
  ├─ #24 Token 估算精确化
  └─ #25 工具并行执行

Phase 6 ═══ 长期愿景
  ├─ #29 自更新系统
  ├─ #30 跨平台 macOS/Linux
  ├─ #31 插件系统
  ├─ #6  Web/extract（Python 能力层）
  ├─ #7  Document to Markdown（Python 能力层）
  └─ #8  Playwright 浏览器自动化（Python 能力层）
```

---

## 四、技术债务速查

| 优先级 | 问题 | 文件 | 建议 |
|:---:|------|------|------|
| 🔴 | `markdownStyleSheet()` 硬编码颜色 | MessageWidget.cpp | 抽到 QSS 变量 |
| 🔴 | Token 估算粗糙 `char*1 + CJK*2/3` | TokenEstimator.cpp | 集成 tiktoken 或 Python sidecar 精确统计 |
| 🟡 | `QEventLoop` 同步等 AI（AiLoopRunner） | AgentLoopController.cpp | 改为全异步信号槽 |
| 🟡 | 重复动作检测基于精确字符串匹配 | AgentOrchestrator.cpp | 改为语义相似度或模糊匹配 |
| 🟡 | MCP 连接器无 TLS/认证 | McpConnector.cpp | 增加 TLS + API Key 透传 |
| 🟡 | 日志可能含 API Key | AppLogger.cpp | 增加日志 sanitization Hook |
| 🟡 | `release/` 目录手动同步 | 构建脚本 | 集成到 CI 自动打包 |

---

## 五、文档约定

- 每完成一个 Phase 中的项目，更新状态为 ✅ 并标注完成日期
- 新增开发项的优先级从 P0 到 P3，按可行性 + 价值排序
- 每天开发结束后更新本文件中的进度
- 技术债务解决后从速查表移除
