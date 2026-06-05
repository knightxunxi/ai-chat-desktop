---
name: text.json_compact
description: 将 JSON 文本压缩为单行紧凑格式，去除所有冗余空白，适合传输或嵌入。
triggers:
  - JSON压缩
  - json compact
  - 压缩JSON
  - compact json
  - 紧凑JSON
  - JSON单行
version: 1.0.0
priority: 20
enabled: true
author: CodeXX Builtin
---

## JSON 压缩工具 (text.json_compact)

**功能**: 将带缩进的 JSON 文本压缩为单行紧凑格式，去除所有不必要的空白字符。

**适用场景**:
- 需要将 JSON 嵌入到 URL 参数、命令行或日志中
- 减少传输体积
- 对比两个 JSON 时先压缩再比较

**调用方式**: 使用 Function Calling 调用 `json_compact`，传入 `{"input": "<JSON文本>"}`。

**注意事项**:
- 输入必须是合法的 JSON 文本
- 压缩后所有空白和换行都会被移除
- 压缩后的 JSON 仍然有效，但可读性会显著降低
