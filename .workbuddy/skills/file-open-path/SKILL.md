---
name: file.open_path
description: 用操作系统默认程序打开指定文件或文件夹，风险等级中等，需用户确认。
triggers:
  - 打开文件
  - 打开文件夹
  - open file
  - open folder
  - 用资源管理器打开
  - 在文件夹中查看
version: 1.0.0
priority: 15
enabled: true
author: CodeXX Builtin
---

## 路径打开工具 (file.open_path)

**功能**: 使用操作系统默认关联程序打开指定的文件或文件夹。

**适用场景**:
- 用户让你帮忙打开某个文件
- 打开项目文件夹在资源管理器中查看
- 打开生成的输出文件

**调用方式**: 使用 Function Calling 调用 `file_open_path`，传入 `{"path": "C:/绝对路径"}`。

**安全边界**:
- 必须提供绝对路径
- 风险等级：中等（涉及系统桌面交互）
- 执行前建议向用户确认要打开的路径
- 打开了外部程序后无法控制后续行为
