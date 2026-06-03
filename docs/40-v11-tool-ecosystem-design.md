# V11 工具生态设计

## 1. 目标

在 V1-V10.3 完成受控文件 Agent、命令执行、技能系统和记忆系统后，扩展 6 个开发者工具，增强项目的实用价值和简历表达力。

## 2. 工具清单

### P0：优先实现

| 工具 ID | 能力 | 风险 | 技术路线 |
|---------|------|------|----------|
| `git.review_diff` | 分析 git diff 变更 | Low | `QProcess` + 白名单 `git diff` |
| `git.review_log` | 查看最近提交记录 | Low | `QProcess` + 白名单 `git log` |
| `logs.summarize` | 搜索/过滤应用日志 | Low | 文件读取 + 关键词匹配 + 脱敏 |

### P1：第二批

| 工具 ID | 能力 | 风险 | 技术路线 |
|---------|------|------|----------|
| `data.csv_read` | 读取工作目录内 CSV | Low | 轻量逗号/引号解析 |
| `data.csv_write` | 写工作目录内 CSV | Medium | 复用 workspace.write + 列校验 |
| `project.find_files` | 按模式搜索项目文件 | Low | 目录遍历 + glob 匹配 |

## 3. 参数 Schema

### git.review_diff
```json
{
  "type": "object",
  "properties": {
    "staged_only": { "type": "boolean", "description": "仅查看已暂存的变更" },
    "max_lines": { "type": "integer", "description": "最大输出行数，默认 200" }
  }
}
```

### git.review_log
```json
{
  "type": "object",
  "properties": {
    "max_count": { "type": "integer", "description": "最大提交数，默认 20" },
    "oneline": { "type": "boolean", "description": "单行模式" }
  }
}
```

### logs.summarize
```json
{
  "type": "object",
  "properties": {
    "keyword": { "type": "string", "description": "搜索关键词" },
    "max_lines": { "type": "integer", "description": "最大返回行数，默认 50" },
    "level": { "type": "string", "enum": ["error", "warning", "info", "all"], "description": "日志级别过滤" }
  }
}
```

### data.csv_read
```json
{
  "type": "object",
  "properties": {
    "path": { "type": "string", "description": "工作目录内相对路径" },
    "max_rows": { "type": "integer", "description": "最大行数，默认 500" },
    "has_header": { "type": "boolean", "description": "首行是否为表头" }
  },
  "required": ["path"]
}
```

### data.csv_write
```json
{
  "type": "object",
  "properties": {
    "path": { "type": "string", "description": "工作目录内相对路径" },
    "rows": { "type": "array", "items": { "type": "array", "items": { "type": "string" } }, "description": "二维字符串数组" },
    "header": { "type": "array", "items": { "type": "string" }, "description": "可选的表头行" }
  },
  "required": ["path", "rows"]
}
```

### project.find_files
```json
{
  "type": "object",
  "properties": {
    "pattern": { "type": "string", "description": "glob 模式，如 *.cpp" },
    "max_results": { "type": "integer", "description": "最大结果数，默认 100" }
  },
  "required": ["pattern"]
}
```

## 4. 架构

```
AgentToolRegistry (注册)
  ├── git.review_diff    → GitReviewService::reviewDiff()
  ├── git.review_log     → GitReviewService::reviewLog()
  ├── logs.summarize     → LogSummaryService::summarize()
  ├── data.csv_read      → CsvDataService::readCsv()
  ├── data.csv_write     → CsvDataService::writeCsv()
  └── project.find_files → ProjectFindService::findFiles()

新增文件：
  src/tools/GitReviewService.h/.cpp
  src/tools/LogSummaryService.h/.cpp
  src/tools/CsvDataService.h/.cpp
  src/tools/ProjectFindService.h/.cpp

新增测试：
  tests/tools/GitReviewServiceTest.cpp
  tests/tools/LogSummaryServiceTest.cpp
  tests/tools/CsvDataServiceTest.cpp
  tests/tools/ProjectFindServiceTest.cpp
```

## 5. 安全约束

- `git.*` 工具：只执行只读命令（diff/log/status），禁止 add/commit/push
- `logs.*` 工具：脱敏 API Key/Token/密码；限制输出行数
- `data.*` 工具：限定工作目录内；CSV 行数上限
- `project.*` 工具：不扫描 .git/build-*/外部目录
