---
name: file.delete_directory
description: 递归删除目录 — 删除目录及其所有内容
trigger_keywords:
  - 删除目录
  - delete directory
  - 删除文件夹
  - delete folder
  - 清理目录
  - 移除目录
  - remove directory
---

# file.delete_directory — 删除目录

递归删除指定目录及其中所有文件和子目录。

## 使用方法

```
file.delete_directory:
  path: 要删除的目录绝对路径
```

## 注意事项

- ⚠️ 不可逆操作！删除后无法恢复
- 系统关键目录（C:\Windows, /etc, /usr 等）受保护无法删除
- 使用前请确认路径正确
