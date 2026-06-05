---
name: file.read_text
description: 读取指定绝对路径的文本文件内容，上限 1 MiB，结果不会自动记录到日志。
triggers:
  - 读取文件
  - 读文件
  - read file
  - 查看文件
  - open file content
  - 文件内容
version: 1.0.0
priority: 25
enabled: true
author: CodeXX Builtin
---

## 文件读取工具 (file.read_text)

**功能**: 读取指定绝对路径的文本文件内容（上限 1 MiB），返回文件正文。文件内容不会写入日志，保护用户隐私。

**适用场景**:
- 用户让你帮忙查看某个文件的代码或配置
- 需要分析日志文件内容
- 读取项目中的文档

**调用方式**: 使用 Function Calling 调用 `file_read_text`，传入 `{"path": "C:/绝对路径/文件名"}`。

**安全边界**:
- 必须提供绝对路径
- 单次读取上限 1 MiB
- 文件内容不会被记录到应用日志
- 结果标记为可能包含敏感内容，向用户展示前请确认
