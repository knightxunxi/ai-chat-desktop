---
name: file.grep
description: 搜索文件内容 — 使用正则表达式在目录或文件中搜索文本匹配
trigger_keywords:
  - 搜索内容
  - search content
  - grep
  - 查找
  - find in files
  - 搜一下
  - 搜索代码
---

# file.grep — 内容搜索

使用正则表达式在指定目录（递归）或文件中搜索文本内容。

## 使用方法

```
file.grep:
  path: 要搜索的目录或文件路径
  pattern: 正则表达式模式
  glob: 可选，按文件名模式过滤（如 *.cpp）
  ignore_case: 可选，是否忽略大小写（默认 false）
```

## 注意事项

- 只读操作，不修改任何文件
- 最多返回 50 条匹配结果
- 大文件（>1 MiB）和二进制文件自动跳过
- 返回格式：file_path:line_number: matched_content
