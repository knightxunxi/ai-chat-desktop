---
name: code-review-cycle
description: 代码审查闭环：扫描变更 → 逐文件审查 → 发现问题 → 自动修复 → 重新审查
version: "1.0"
priority: 90
enabled: true
triggers:
  - 审查代码
  - code review
  - 代码审查
  - review code
  - 检查代码
  - 代码检查
  - audit code
---

# 代码审查闭环 (Code Review Cycle)

当此技能激活时，你对项目代码进行系统性审查，发现问题自动修复，修复后重新验证。

## 审查流程

### 1. 变更扫描
- 用 `command.bash {"command":"git status --short"}` 查看变更文件
- 用 `command.bash {"command":"git diff --stat"}` 查看变更统计

### 2. 逐文件审查
对每个变更文件检查以下问题：
- **内存安全**：是否有未初始化的指针、悬空引用、内存泄漏
- **异常安全**：资源是否正确释放（RAII）
- **逻辑错误**：条件判断、循环边界、空指针检查
- **代码风格**：命名、注释、缩进一致性
- **Qt 最佳实践**：信号槽连接是否正确、parent 设置是否正确
- **头文件包含**：是否有多余或缺失的 include

### 3. 发现问题的处理
- 小问题（命名、风格）→ 直接用 `file.edit_text` 修复
- 中问题（逻辑、空指针）→ 修复后说明原因
- 大问题（架构缺陷）→ 在报告中标注，不自动修复

### 4. 自动验证
修复后重新构建+测试：
- `command.bash {"command":"cmake --build build"}`
- `command.bash {"command":"ctest --test-dir build --output-on-failure"}`

### 5. 输出审查报告
```
📋 代码审查报告
审查文件: N 个
发现问题: X 个（已修复 Y 个，需关注 Z 个）

✅ 已修复:
- AgentOrchestrator.cpp:201: 未检查空指针
- InputSimulator.h:45: 缺少 override 关键字

⚠️ 需关注:
- ApplicationController.cpp: 函数过长（>200 行），建议拆分

构建: ✅ | 测试: 65/65 ✅
```

## 规则
- 修改后自动构建验证
- 不做破坏性重构
- 每个修复附带简短理由
