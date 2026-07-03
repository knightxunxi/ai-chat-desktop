# V16 体验增强 — 开发规划

**创建日期**: 2026-06-04
**前置依赖**: V15 全部完成 (62/62 测试通过)
**基线**: MessageWidget 已有基础 Markdown 渲染 (QTextDocument::MarkdownDialectGitHub)；Agent 循环已有信号轮次计数但无可视化

---

## 一、Chat 方向优化可选项

### 1.1 现状

| 组件 | 能力 | 局限 |
|------|------|------|
| `MessageWidget::addTextSegment()` | QTextDocument::setMarkdown → HTML → QLabel RichText | 仅 Assistant 走 RichText，User 走 PlainText |
| `MessageWidget::addCodeBlock()` | QPlainTextEdit 独立渲染 + 语言标签 + 复制按钮 | 无语法着色、无行号 |
| `MessageWidget::splitAssistantContent()` | 手写行扫描器，只认 ``` 标记 | 不识别 4空格缩进代码、表格、引用 |
| `ChatView::updateLastAssistantMessage()` | 流式更新时完整重建全部 widget | 性能差，每次 Delta 触发全量解析+重建 |
| `markdownStyleSheet()` | 硬编码 CSS 颜色值 | 不支持暗色主题 |

### 1.2 优化项（按投入产出比排序）

| ID | 优化项 | 说明 | 改动文件数 | 预估 |
|:--:|--------|------|:---:|:---:|
| **CH-1** | 流式增量更新优化 | Delta 到达时不重建结构，只更新最后一个文本段/代码块的 content | 2 | 0.5天 |
| **CH-2** | 扩展 splitAssistantContent 支持表格+引用 | 识别 `\|` 表格行和 `>` 引用行，拆分为独立 ContentPart::Table/Quote | 2 | 0.5天 |
| **CH-3** | 代码块语法着色 | 实现 QSyntaxHighlighter 子类，对 cpp/python/js/bash/sql 做关键词匹配着色 | 2 | 1天 |
| **CH-4** | CSS 主题化 | 将 markdownStyleSheet() 的硬编码颜色改为从 qApp 读取或支持亮/暗切换 | 2 | 0.5天 |
| **CH-5** | User 消息也走 RichText | 用户消息内的 `**粗体**`、行内代码等也应该渲染 | 1 | 0.25天 |
| **CH-6** | 代码块行号 | QPlainTextEdit 左侧添加 lineNumberArea | 2 | 0.5天 |
| **CH-7** | 消息右键菜单 | 复制/删除/重新生成/引用回复 | 2 | 1天 |
| **CH-8** | 消息编辑 | 编辑已发送消息 → 重新生成 AI 回复 | 3 | 1.5天 |
| **CH-9** | 图片粘贴支持 | 聊天中粘贴截图 → OCR + 多模态 API | 3 | 1天 |
| **CH-10** | 打字指示器 | AI 思考时动态动画反馈 | 1 | 0.25天 |

---

## 二、Agent 方向优化可选项

### 2.1 现状

| 组件 | 能力 | 局限 |
|------|------|------|
| `ApplicationController` | 仅 emit agentLoopIterationUpdated (轮次计数器) 到状态栏 | AI 推理/工具选择/观测数据全部不可见 |
| `AgentLoopCallbacks` | std::function 回调，stepStarted/stepFinished | **未被上层使用** |
| `agentLoopSkillSummary` 信号 | V13.3 已定义 | **MainWindow 未连接任何槽** |
| `executePlanAndReportToChat()` | 追加 Markdown 结果到聊天 | 仅显示最终汇总，不显示过程 |
| `continueAgentLoop()` | 循环提示词作为 systemPrompt | 用户完全看不到 AI 收到的 prompt |

### 2.2 优化项（按投入产出比排序）

| ID | 优化项 | 说明 | 改动文件数 | 预估 |
|:--:|--------|------|:---:|:---:|
| **AG-1** | 思考步骤卡片（ApplicationController 新增信号） | 新增 agentLoopStep 信号，emit AI 推理+工具选择+工具结果；ChatView 中插入可折叠步骤卡片 | 4 | 1天 |
| **AG-2** | 连接 agentLoopSkillSummary | MainWindow 新增连接，每次 Agent 完成时在聊天区显示技能使用摘要 | 1 | 0.25天 |
| **AG-3** | 工具调用实时状态 | handleToolUseBlockComplete / handleRequestFinished 中发射详细状态信号 | 2 | 0.5天 |
| **AG-4** | Agent 执行回放 | 持久化每轮 step record → 历史会话可回看完整 Agent 过程 | 3 | 1.5天 |
| **AG-5** | 思考 prompt 调试视图 | 可选的开发者模式：在聊天中显示 AI 实际收到的完整 prompt | 2 | 0.5天 |
| **AG-6** | 人工确认 UI 增强 | 高危操作确认弹窗加入风险颜色、操作预览、参数展示 | 2 | 0.5天 |
| **AG-7** | Agent 会话恢复 | 中断后的 Agent 可从上次状态 resume | 3 | 1.5天 |
| **AG-8** | MCP 服务器管理 UI | 图形化增删改 MCP 服务器配置 | 2 | 1天 |

---

## 三、优先级排序（按版本划分）

### 选择逻辑

- P0 = 改动少 + 效果大 + 不依赖其他项
- P1 = 效果大但改动多，或依赖 P0 完成
- P2 = 锦上添花，用户体验细节

### V16.1 — 快速见效包（P0，2 天）

| ID | 方向 | 做什么 | 预估 |
|:--:|------|------|:---:|
| CH-1 | Chat | 流式增量更新优化 | 0.5天 |
| CH-5 | Chat | User 消息 RichText | 0.25天 |
| AG-2 | Agent | 连接 agentLoopSkillSummary | 0.25天 |
| AG-3 | Agent | 工具调用实时状态信号 | 0.5天 |
| AG-1 | Agent | 思考步骤卡片（核心） | 0.5天（依赖 AG-3） |

### V16.2 — Markdown 增强包（P1，1.5 天）

| ID | 方向 | 做什么 | 预估 |
|:--:|------|------|:---:|
| CH-2 | Chat | 扩展表格+引用渲染 | 0.5天 |
| CH-3 | Chat | 代码块语法着色 | 1天 |

### V16.3 — 交互体验包（P2，2 天）

| ID | 方向 | 做什么 | 预估 |
|:--:|------|------|:---:|
| CH-7 | Chat | 消息右键菜单 | 1天 |
| CH-4 | Chat | CSS 主题化 | 0.5天 |
| AG-5 | Agent | 思考 prompt 调试视图 | 0.5天 |

### V16.4 — 深度功能包（P3，3 天）

| ID | 方向 | 做什么 | 预估 |
|:--:|------|------|:---:|
| CH-8 | Chat | 消息编辑 + 重新生成 | 1.5天 |
| AG-6 | Agent | 人工确认 UI 增强 | 0.5天 |
| AG-4 | Agent | Agent 执行回放 | 1天（部分实现） |

### 暂缓（待评估）

| ID | 原因 |
|:--:|------|
| CH-6 | 代码块行号 — QPlainTextEdit 左侧绘制复杂，可等第三方库 |
| CH-9 | 图片粘贴 — 依赖多模态 API 稳定性和上传链路 |
| CH-10 | 打字指示器 — 纯视觉点缀，优先级最低 |
| AG-7 | Agent 会话恢复 — 涉及序列化循环状态，复杂度高 |
| AG-8 | MCP UI — MCP 生态还不够成熟，等协议稳定 |

---

## 四、版本间依赖

```
                     V16.1 快速见效
                    /              \
            AG-2(技能摘要)     AG-3(实时信号)
                    \              /
                     AG-1(步骤卡片)
                          │
                     V16.2 Markdown增强
                    /              \
            CH-2(表格引用)     CH-3(语法着色)
                          │
                     V16.3 交互体验
                    /              \
            CH-7(右键菜单)    CH-4(主题化)
                          │
                     V16.4 深度功能
                    /              \
            CH-8(消息编辑)    AG-6(确认UI)
```

---

## 五、验收标准

| AC | 内容 |
|----|------|
| AC-1 | Agent 循环中每一步都能在聊天界面看到：推理、工具名、执行结果（可折叠卡片） |
| AC-2 | 流式回复不再每次 Delta 全量重建，CPU 占用明显下降 |
| AC-3 | 会话完成后技能使用摘要自动出现在聊天末尾 |
| AC-4 | Markdown 表格正确渲染为 HTML table |
| AC-5 | 代码块有基本语法着色（cpp/python/js/bash） |
| AC-6 | 消息右键菜单包含复制/重新生成 |
| AC-7 | `ctest` 全量零回归 |
