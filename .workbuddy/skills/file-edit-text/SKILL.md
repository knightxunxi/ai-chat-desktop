---
name: file.edit_text
description: 精确编辑文件内容 — 在文件中查找 old_str 并替换为 new_str
trigger_keywords:
  - 编辑文件
  - edit file
  - 替换文本
  - replace text
  - 修改代码
  - modify code
  - 改成
  - 改为
---

# file.edit_text — 精确编辑文件

使用字符串精确匹配替换的方式编辑文件，不会影响文件其他部分。

## 使用方法

```
file.edit_text:
  path: 要编辑的文件绝对路径
  old_str: 要被替换的文本（必须在文件中恰好出现一次）
  new_str: 替换后的新文本
```

## 注意事项

- old_str 必须在文件中恰好出现一次，如果出现多次会报错（需要提供更多上下文）
- 文件会被就地修改，无备份
- 适用于文本文件
