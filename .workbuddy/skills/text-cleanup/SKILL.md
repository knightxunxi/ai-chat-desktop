---
name: text.cleanup
description: 清理纯文本：统一换行符、压缩连续空行、去除首尾空白，产生干净统一的文本。
triggers:
  - 文本清理
  - 清理文本
  - text cleanup
  - clean text
  - 统一换行
  - 去除空行
  - normalize text
version: 1.0.0
priority: 15
enabled: true
author: CodeXX Builtin
---

## 文本清理工具 (text.cleanup)

**功能**: 对纯文本进行基础清理——统一换行符（CRLF→LF）、压缩连续空行（≥3行→2行）、去除首尾空白。

**适用场景**:
- 从不同系统粘贴的文本换行混乱（Windows/Linux/Mac）
- 文本中有大段空白需要压缩
- 需要统一文本格式后再做后续处理

**调用方式**: 使用 Function Calling 调用 `text_cleanup`，传入 `{"input": "<文本>"}`。

**注意事项**:
- 这是纯文本处理工具，不要对 Markdown 或代码使用——请用 `text.markdown_cleanup`
- 不会修改文本语义内容，仅处理空白字符
