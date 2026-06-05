---
name: text.markdown_cleanup
description: 清理 Markdown 文本中的冗余空白和多余空行，同时保留代码块内容不变。
triggers:
  - Markdown清理
  - 清理markdown
  - markdown cleanup
  - md整理
  - 格式化markdown
  - markdown format
version: 1.0.0
priority: 15
enabled: true
author: CodeXX Builtin
---

## Markdown 整理工具 (text.markdown_cleanup)

**功能**: 清理 Markdown 文本中的冗余空白和多余连续空行，代码块内容完全保留不变。

**适用场景**:
- 粘贴的 Markdown 有大量奇怪的空白字符
- 从网页复制的 Markdown 格式混乱
- 合并多个文档后需要统一格式

**调用方式**: 使用 Function Calling 调用 `markdown_cleanup`，传入 `{"input": "<Markdown文本>"}`。

**安全保证**:
- 代码块（``` ... ```）内容**完全不被修改**
- 不会删除任何实质内容，仅清理空白
- 适用于包含代码片段的文档
