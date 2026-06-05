---
name: file.save_text
description: 将文本内容保存到指定绝对路径文件，不会覆盖已存在文件，风险等级中等。
triggers:
  - 保存文件
  - 写入文件
  - save file
  - 输出到文件
  - write file
  - 存为文件
version: 1.0.0
priority: 20
enabled: true
author: CodeXX Builtin
---

## 文件保存工具 (file.save_text)

**功能**: 将文本内容保存到指定的绝对路径文件中。如果目标文件已存在，操作会被拒绝（不会覆盖）。

**适用场景**:
- 用户让你把生成的内容保存为文件
- 导出格式化后的 JSON / Markdown / 纯文本

**调用方式**: 使用 Function Calling 调用 `file_save_text`，传入 `{"path": "C:/绝对路径/文件名", "content": "<文本内容>"}`。

**安全边界**:
- 必须提供绝对路径
- 不会覆盖已存在的文件
- 风险等级：中等（涉及本地文件写入）
- 操作前建议向用户确认路径
