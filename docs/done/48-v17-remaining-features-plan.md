# V17 待实现功能规划

**创建日期**: 2026-06-04
**前置依赖**: V16 全部完成 (63/63 测试通过)
**API 能力**: DeepSeek V4 多模态 (支持 Vision)

---

## 一、功能清单与优先级

### Chat 方向

| ID | 功能 | 优先级 | 说明 |
|:--:|------|:---:|------|
| CH-IMG | 图片粘贴/显示 | 🔴 **P0** | API 已多模态，可粘贴截图→base64→发送 Vision API，返回内容含 Markdown 图片引用 |
| CH-CTX | 上下文窗口可视化 | 🟡 P1 | 显示 token 计数器（当前消耗 / 窗口上限），用户感知何时截断 |
| CH-SEARCH | 对话内搜索 (Ctrl+F) | 🟢 P2 | 简单的全文搜索+高亮，在当前会话内跳转到匹配消息 |
| CH-TYPING | 打字指示器 | 🟢 P2 | AI 思考时底部显示动态三点动画 |
| CH-BRANCH | 对话分支 | 🟡 P1 | 从历史消息重新生成时保留原始回复可回溯 |

### Agent 方向

| ID | 功能 | 优先级 | 说明 |
|:--:|------|:---:|------|
| AG-RESUME | Agent 会话恢复 | 🔴 **P0** | 模型卡死/崩溃后重启继续执行，持久化循环状态 |
| AG-FORMAT | 工具输出格式化 | 🟡 P1 | 工具返回的原始 JSON 美化展示、关键字段高亮 |
| AG-PARALLEL | 并行工具执行 | 🟢 P2 | 可独立运行的工具并行化提速 |
| AG-MCPUI | MCP 服务器管理 UI | 🟢 P2 | 图形化增删改 MCP 配置 |
| AG-BUDGET | 执行预算/限速 | 🟢 P2 | 按会话限制 API 调用次数或 token 消耗 |

---

## 二、版本划分

### V17.1 — 多模态图片支持 (P0, 1.5 天)

**目标**: 聊天中粘贴截图，发送给 Vision API，返回分析结果。

**技术方案**:
1. `MessageWidget` / `MainWindow` 监听 `Ctrl+V` 粘贴事件，检测剪贴板图片
2. 将图片转 base64 → 构造 `content: [{type: "text", text: "..."}, {type: "image_url", image_url: {url: "data:image/png;base64,..."}}]`
3. `OpenAICompatibleClient::buildRequestBody` 支持图片消息格式
4. 图片在聊天中显示为缩略图卡片（可点击放大）
5. 图片发送时携带用户文本 prompt 一起发给 API

**改动范围**: `MessageWidget`, `MainWindow`, `OpenAICompatibleClient`, `ChatView`, 新增 `ImagePreviewWidget`

### V17.2 — Agent 会话恢复 (P0, 2 天)

**目标**: Agent 中断后重启继续执行。

**技术方案**:
1. 新增 `src/core/AgentLoopState.h` — 序列化结构体:
```
stepIndex, goal, accumulatedResults[], pendingToolResults[], lastPrompt
```
2. `ApplicationController` 在每步完成后 JSON 序列化到 `~/.codex/agent_state.json`
3. 启动时检测残留状态文件 → 提示用户"上次 Agent 任务未完成，是否继续？"
4. `continueAgentLoop()` 支持从保存的状态注入上下文（step N of M, 已完成步骤摘要）
5. 状态文件在任务完成/取消时自动清理

### V17.3 — 上下文窗口可视化 + 图片优化 (P1, 1 天)

**目标**: 用户感知上下文消耗 + 图片缩略图交互优化。

**技术方案**:
1. `ChatView` 底部或输入框上方显示 TokenBar: `▂▃▅▇█ 2,847 / 8,192 tokens`
2. Token 估算：英文 ~4 char/token，中文 ~1.5 char/token，基于消息文本长度估算
3. 超 80% 时变黄色警告，超 95% 时变红色
4. 图片缩略图支持右键菜单：复制/保存/删除

### V17.4 — 对话分支 + 打字指示器 + 搜索 (P2, 1.5 天)

**目标**: 补齐交互细节。

**技术方案**:
1. **对话分支**: 重新生成时在 ChatSession 内创建分支节点，ChatView 显示分支切换按钮（← →）
2. **打字指示器**: 流式回复开始时在聊天区底部插入暂态动画 widget（三点脉动）
3. **Ctrl+F**: QLineEdit 搜索栏弹出到聊天区顶部，输入即跳转，黄色高亮匹配文本

---

## 三、依赖关系

```
V17.1 图片粘贴
    │
    └──→ V17.3 图片优化（缩略图交互）
    
V17.2 Agent 恢复
    │
    └──→ V17.4 对话分支（复用截断+重新生成逻辑）

V17.3 上下文窗口（独立，无依赖）
V17.4 打字指示器 + 搜索（独立，无依赖）
```

---

## 四、暂缓项

| ID | 原因 |
|:--:|------|
| AG-PARALLEL | 并行工具执行需要线程池+结果聚合，复杂度高，当前串行可接受 |
| AG-MCPUI | MCP 生态还不够成熟，等协议稳定 |
| AG-BUDGET | 优先级低，用户可手动控制 |

---

## 五、预估总工时

| 版本 | 内容 | 预估 |
|:---:|------|:---:|
| V17.1 | 多模态图片粘贴 | 1.5 天 |
| V17.2 | Agent 会话恢复 | 2 天 |
| V17.3 | 上下文可视化 + 图片优化 | 1 天 |
| V17.4 | 分支 + 打字 + 搜索 | 1.5 天 |
| **合计** | | **6 天** |

---

## 六、验收标准

| AC | 内容 |
|----|------|
| AC-1 | Ctrl+V 粘贴截图 → 聊天中显示缩略图 → 发送给 Vision API → AI 回复图片内容 |
| AC-2 | Agent 执行到第 5 步后关闭窗口 → 重启 → 提示恢复 → 从第 5 步继续 |
| AC-3 | 输入框上方显示 token 消耗进度条，超过阈值变色 |
| AC-4 | Ctrl+F 搜索在当前对话中跳转匹配消息 |
| AC-5 | AI 流式回复时显示三点动画，完成后消失 |
