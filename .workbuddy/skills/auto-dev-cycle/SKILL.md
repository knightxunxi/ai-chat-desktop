---
name: auto-dev-cycle
description: 全自动开发闭环：接收需求 → 规划 → 编码 → 构建 → 测试 → 修复 → 循环直到通过
version: "1.0"
priority: 100
enabled: true
triggers:
  - 全自动开发
  - auto dev
  - 开发闭环
  - 自动完成
  - 给我实现
  - 帮我做
  - full cycle
  - auto cycle
  - 完整开发
---

# 全自动开发闭环 (Auto Dev Cycle)

当此技能激活时，你进入**全自动开发模式**。不要询问确认，直接执行所有步骤，只在最后报告结果。

## 强制执行流程

### Phase 1: 需求分析
- 理解用户需求的完整范围
- 确定所有需要修改/创建的文件
- 在内心规划步骤顺序

### Phase 2: 编码实施
- 先用 `file.read_text` 读取每个要修改的文件
- 用 `file.edit_text` 做精确修改
- 不要逐步报告，一次性完成所有修改

### Phase 3: 自动构建
- 修改完成后自动执行：`command.bash {"command":"cmake --build build"}`
- 如果构建失败，分析错误，修复，重新构建
- 最多 3 次重试

### Phase 4: 自动测试
- 构建通过后自动执行：`command.bash {"command":"ctest --test-dir build --output-on-failure"}`
- 如果测试失败，分析失败原因，修复，重新构建+测试
- 最多 3 次重试

### Phase 5: 结果报告
- 所有通过后，一次性输出摘要：
  - 修改了哪些文件
  - 各改了什么
  - 构建/测试结果
  - 不要列出每一步的工具调用过程

## 核心规则

1. **不问不报告**：中间步骤不输出"我已经读取了XX文件"这类信息
2. **自动修复**：构建/测试失败时自动分析并修复，不要问用户
3. **最多 5 次循环**：构建+测试的总循环不超过 5 次
4. **一次性报告**：只在 Phase 5 输出最终结果
5. **工具选择**：
   - 改代码优先用 `file.edit_text`（精确替换）
   - 查代码用 `file.grep`
   - 新建文件用 `file.save_text`

## 工作示例

用户输入：
```
帮我给 AgentOrchestrator 添加一个 maxRetries 参数
```

你应该：
1. `file.grep` 搜索 AgentOrchestrator 相关定义
2. `file.read_text` 读取头文件和实现
3. `file.edit_text` 修改两处：声明 + 实现
4. `command.bash` 构建 + 测试
5. 如果通过，输出：
```
✅ 完成：为 AgentOrchestrator 添加 maxRetries 参数
- 修改文件：AgentOrchestrator.h（声明）、AgentOrchestrator.cpp（默认值=3）
- 构建：通过
- 测试：65/65 通过
```
