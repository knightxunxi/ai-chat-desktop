# CodeXX 后续四方向开发规划

> 创建日期：2026-07-04
> 范围：仅规划当前确认的四个方向，不展开长期愿景项。
> 目标：明确先后顺序、并行关系、验收标准和可参考的开源或成品。

---

## 1. 总体策略

后续开发按一条主线和三条支线推进：

```text
主线：Python 能力层闭环
  -> Agent 实用能力
  -> 工程化质量
  -> 核心复杂度治理

支线 A：UI 小体验可与主线并行
支线 B：CI、日志脱敏、文档维护可与主线并行
支线 C：架构拆分必须等测试兜底后再做
```

核心判断：

- 先让 Python sidecar 成为可切换 AI 后端，否则后续 Web、token、文档解析都会缺少统一能力入口。
- 再做 Agent 可感知能力，保证产品体验继续前进。
- 同时补工程化质量，尤其是 Agent E2E 测试和 CI。
- 最后处理复杂度较高的拆分和异步化，避免没有测试保护时重构主流程。

---

## 2. 阶段顺序和并行关系

| 阶段 | 方向 | 主要任务 | 是否可并行 | 前置条件 | 建议周期 |
|------|------|----------|------------|----------|----------|
| S1 | Python 能力层闭环 | `#3` PythonSidecarAIClient、`#4` 多厂商适配、`#5` 精确 token | 主线串行，内部小步并行 | 已有 sidecar 骨架和 QProcess 客户端 | 2-3 天 |
| S2 | Agent 实用能力 | `#11` 快捷键、`#12` 输入增强、`#9` WebSearch、`#10` Token 预算追踪 | UI 小项可并行，WebSearch 依赖 S1 更稳 | Agent 循环可用，工具注册稳定 | 2-3 天 |
| S3 | 工程化质量 | `#21` E2E 测试、`#20` 日志脱敏、`#13` 自动打包 CI、`#14` 使用统计 | 可与 S2 并行，但 E2E 优先 | 主要能力路径明确 | 3-5 天 |
| S4 | 核心复杂度治理 | `#19` AC 拆分、异步化、重复动作检测、主题变量化 | 不建议和 S1/S2 并行改主流程 | E2E 测试和 CI 基本可用 | 4-6 天 |

推荐执行顺序：

1. S1-1：PythonSidecarAIClient 接入 `AIClient`。
2. S1-2：配置切换 direct C++ backend / Python sidecar backend。
3. S1-3：Python 多厂商适配和精确 token 统计。
4. S2-1：快捷键系统和输入区增强。
5. S2-2：Token 预算追踪。
6. S2-3：WebSearch 工具。
7. S3-1：MockApiClient + Agent E2E 测试。
8. S3-2：日志脱敏、自动打包 CI、使用统计。
9. S4-1：ApplicationController 继续拆分。
10. S4-2：异步化和重复动作检测优化。
11. S4-3：主题颜色变量化。

---

## 3. S1：Python 能力层闭环

### 3.1 目标

让 Python sidecar 从“可 ping、可 token.count”升级为“可选 AI 后端”：

```text
ApplicationController / AgentLoop
  -> AIClient 接口
  -> OpenAICompatibleClient 或 PythonSidecarAIClient
  -> Python sidecar model.chat / token.count
```

### 3.2 任务拆分

| 任务 | 内容 | 并行建议 |
|------|------|----------|
| S1-1 | 新增 `PythonSidecarAIClient`，实现 `AIClient` 接口 | 主线，先做 |
| S1-2 | 增加配置项，允许选择 direct / sidecar backend | 可与 S1-1 后半并行 |
| S1-3 | Python `model.chat` 对接 OpenAI-compatible API | 依赖 S1-1 协议稳定 |
| S1-4 | Python `token.count` 替换为精确 tokenizer | 可和 S1-3 并行 |
| S1-5 | C++/Python 双端错误分类和超时测试 | 与 S1-3/S1-4 同步补 |

### 3.3 验收标准

- 配置 direct backend 时，现有 C++ `OpenAICompatibleClient` 路径不变。
- 配置 sidecar backend 时，普通聊天和 Agent 工具循环可通过 Python sidecar 发起模型请求。
- Python sidecar 退出、超时、返回错误时，UI 有明确错误，Agent 不假装任务完成。
- C++ 测试覆盖协议、超时、错误路径。
- Python 单元测试覆盖 `model.chat`、`token.count`、结构化错误。

### 3.4 可参考项目

| 参考 | 链接 | 借鉴点 |
|------|------|--------|
| OpenAI Python SDK | https://github.com/openai/openai-python | OpenAI-compatible 请求格式、流式响应、错误结构 |
| LiteLLM | https://github.com/BerriAI/litellm | 多厂商适配思路；建议作为后续可选依赖，不作为第一阶段强依赖 |
| tiktoken | https://github.com/openai/tiktoken | 精确 token 统计 |

---

## 4. S2：Agent 实用能力

### 4.1 目标

提升用户每天能感知到的效率，同时验证 Agent 工具循环稳定性。

### 4.2 任务拆分

| 任务 | 内容 | 并行建议 |
|------|------|----------|
| S2-1 | 快捷键系统：Ctrl+N/K/L/Enter/Esc | 可独立并行 |
| S2-2 | 输入区增强：历史、Ctrl+Enter、@文件 | 可与 S2-1 并行，但都改 UI 输入区时需要错开文件 |
| S2-3 | Token 预算追踪：每轮统计、预算提示、边际收益检测 | 可与 UI 小项并行 |
| S2-4 | WebSearch 工具 | 建议在 S1 sidecar 后做，便于后续放到 Python 能力层 |

### 4.3 验收标准

- 快捷键不破坏现有发送、搜索、复制、编辑消息行为。
- 输入历史和 `@文件` 不改变普通聊天和 Agent 消息语义。
- Token 预算展示能区分当前轮、累计会话、Agent 循环消耗。
- WebSearch 工具返回结构化结果，失败时返回可观测错误。

### 4.4 可参考项目或产品

| 参考 | 链接 | 借鉴点 |
|------|------|--------|
| SearXNG | https://github.com/searxng/searxng | 自托管元搜索，可作为 WebSearch 后端参考 |
| Tavily API | https://docs.tavily.com/ | 面向 Agent 的搜索 API 形态和结果结构 |
| Visual Studio Code Command Palette | https://code.visualstudio.com/docs/getstarted/userinterface#_command-palette | Ctrl+K/Ctrl+Shift+P 类命令面板交互 |
| Raycast | https://www.raycast.com/ | 命令式效率工具的交互参考 |

---

## 5. S3：工程化质量

### 5.1 目标

建立能保护后续重构的测试、CI、日志和统计能力。

### 5.2 任务拆分

| 任务 | 内容 | 并行建议 |
|------|------|----------|
| S3-1 | MockApiClient + Agent E2E 循环测试 | 优先做，不建议延后 |
| S3-2 | 日志脱敏增强，追加前走 `SensitiveFilterHook` | 可与 S2 并行 |
| S3-3 | 自动打包 CI：Release build + windeployqt + zip | 可独立并行 |
| S3-4 | 使用统计面板：SQLite 统计 + UI 展示 | 可独立并行 |

### 5.3 验收标准

- E2E 测试能覆盖“模型返回工具调用 -> 工具执行 -> 观察结果 -> 下一轮继续”。
- MockApiClient 能模拟成功、失败、截断、空响应、工具调用。
- CI 能在 Windows 上构建、测试、打包。
- 日志脱敏能覆盖 API Key、Bearer Token、常见密钥格式。
- 使用统计不记录敏感内容，只记录计数、耗时、token、工具类型等非敏感数据。

### 5.4 可参考项目或文档

| 参考 | 链接 | 借鉴点 |
|------|------|--------|
| Qt Test | https://doc.qt.io/qt-6/qttest-index.html | Qt 官方测试框架思路，可参考但不强制迁移 |
| CTest | https://cmake.org/cmake/help/latest/manual/ctest.1.html | 当前 CMake 测试入口 |
| GitHub Actions | https://docs.github.com/actions | CI 工作流 |
| jurplel/install-qt-action | https://github.com/jurplel/install-qt-action | GitHub Actions 中安装 Qt |
| windeployqt | https://doc.qt.io/qt-6/windows-deployment.html | Windows Qt 打包部署 |

---

## 6. S4：核心复杂度治理

### 6.1 目标

在测试和 CI 兜底后，降低主控和 Agent 循环复杂度，避免后续功能继续堆到 `ApplicationController` 和循环函数里。

### 6.2 任务拆分

| 任务 | 内容 | 并行建议 |
|------|------|----------|
| S4-1 | `ApplicationController` 继续拆分：`StreamCoordinator`、`MediaCoordinator` | 必须在 E2E 后做 |
| S4-2 | `AgentLoopController::executeLoop` 异步化，减少同步等待 | 不与 S4-1 同时改主流程 |
| S4-3 | `AgentOrchestrator` 重复动作检测优化 | 可在 S4-1 后并行 |
| S4-4 | `MessageWidget` 主题颜色变量化 | 可独立并行，风险较低 |

### 6.3 验收标准

- `ApplicationController` 对外行为不变，测试不需要大面积重写。
- 拆分后每个 Coordinator 职责单一，有明确输入输出。
- Agent 循环异步化后没有嵌套 `QEventLoop` 卡 UI 的风险。
- 重复动作检测能减少重复执行，但不会误杀正常连续操作。
- 主题变量化后暗色主题、Markdown、代码块、表格显示一致。

### 6.4 可参考项目或文档

| 参考 | 链接 | 借鉴点 |
|------|------|--------|
| Qt State Machine | https://doc.qt.io/qt-6/statemachine-api.html | 复杂状态迁移建模 |
| QtConcurrent / QFuture | https://doc.qt.io/qt-6/qtconcurrent-index.html | 异步任务和后台计算 |
| Martin Fowler Strangler Fig | https://martinfowler.com/bliki/StranglerFigApplication.html | 渐进式拆分旧逻辑 |
| Refactoring Guru Command Pattern | https://refactoring.guru/design-patterns/command | 工具执行、撤销、命令封装思路 |

---

## 7. 并行开发矩阵

| 组合 | 是否建议并行 | 原因 |
|------|:---:|------|
| S1 PythonSidecarAIClient + S2 快捷键 | ✅ | 分属 Service/App 与 UI，冲突小 |
| S1 PythonSidecarAIClient + S2 WebSearch | ⚠️ | WebSearch 最好等 sidecar AI 后端边界稳定 |
| S2 快捷键 + S2 输入区增强 | ⚠️ | 都改 `MainWindow` 输入区，需拆分文件和验证 |
| S2 Token 预算 + S3 使用统计 | ✅ | 数据采集可复用，但需先定义指标名 |
| S3 E2E 测试 + S4 AC 拆分 | ❌ | 应先完成 E2E，再拆主流程 |
| S3 CI + S2/S1 功能开发 | ✅ | CI 可独立推进 |
| S4 AC 拆分 + S4 异步化 | ❌ | 都动主流程，必须串行 |
| S4 主题变量化 + 任意后端任务 | ✅ | UI 渲染样式独立，风险较低 |

---

## 8. 建议近期任务包

### Task Pack A：Python 后端闭环

- `#3 PythonSidecarAIClient`
- 配置切换
- Python `model.chat`
- C++/Python 协议测试

完成后进入 Task Pack B。

### Task Pack B：用户体验小闭环

- 快捷键系统
- 输入历史和 Ctrl+Enter
- Token 预算追踪基础版

可与 Task Pack C 的 CI 子项并行。

### Task Pack C：质量兜底

- MockApiClient
- Agent E2E 循环测试
- GitHub Actions Windows 构建测试
- 日志脱敏增强

完成后再进入 Task Pack D。

### Task Pack D：复杂度治理

- `ApplicationController` 拆分
- `AgentLoopController` 异步化
- 重复动作检测优化
- MessageWidget 主题变量化

---

## 9. 暂缓事项

以下事项暂不进入近期四方向开发：

- 自更新系统。
- 角色扮演系统。
- 双 Agent 互审。
- 游戏自动化。
- 插件系统。
- 跨平台 macOS/Linux。

这些方向价值存在，但会分散当前主线。当前主线应优先保证 Agent 可执行、可测试、可扩展。
