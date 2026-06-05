---
name: file.list_directory
description: 列出指定绝对路径文件夹下的条目（最多 200 项），方便快速了解目录结构。
triggers:
  - 列出文件夹
  - 列表目录
  - list directory
  - 目录内容
  - 文件夹内容
  - ls
  - dir
version: 1.0.0
priority: 25
enabled: true
author: CodeXX Builtin
---

## 文件夹列出工具 (file.list_directory)

**功能**: 列出指定绝对路径文件夹下的文件和子文件夹条目（最多 200 项）。

**适用场景**:
- 用户让你看看某个目录下有什么文件
- 分析项目结构前先了解文件布局
- 检查构建输出目录

**调用方式**: 使用 Function Calling 调用 `file_list_directory`，传入 `{"path": "C:/绝对路径/文件夹"}`。

**安全边界**:
- 必须提供绝对路径
- 最多返回 200 个条目
- 结果可能暴露本地文件名，向用户展示前请确认
