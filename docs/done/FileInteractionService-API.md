# FileInteractionService API 文档

> CodeXX 内置文件交互服务及 Agent Tool Registry 映射  
> 状态：**Active**（UI 入口已移除，底层 API 保留）  
> 日期：2026-06-05

---

## 1. 概述

`FileInteractionService` 是 CodeXX 的受控本地文件交互服务命名空间，提供文件读写、目录遍历、路径校验等核心能力。原本通过 `FileToolsDialog` UI 调用，现改为通过 **Agent Skill**（`.workbuddy/skills/file-*/SKILL.md`）或 **Function Calling** 由 AI 直接调用。

### 文件路径

| 类型 | 路径 |
|------|------|
| 服务实现 | `src/tools/core/FileInteractionService.h` |
| 源文件 | `src/tools/core/FileInteractionService.cpp` |
| UI 对话框（已移除） | `src/ui/FileToolsDialog.h/.cpp` |
| Agent 工具注册 | `src/tools/AgentToolRegistry.cpp` → `registerFileTools()` |
| Skill 定义 | `.workbuddy/skills/file-read-text/SKILL.md` 等 |

---

## 2. API 接口

### 2.1 readTextFile

```cpp
ToolResult readTextFile(const QString &filePath, qint64 maxBytes = DefaultMaxTextFileBytes);
```

读取用户指定的文本文件内容。

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `filePath` | `const QString &` | — | 文件绝对路径 |
| `maxBytes` | `qint64` | `1 MiB (1048576)` | 最大读取字节数，防止 UI 卡死 |

| 返回值 | 字段 | 类型 | 说明 |
|--------|------|------|------|
| `ToolResult` | `ok` | `bool` | 是否成功 |
| | `output` | `QString` | 文件文本内容（成功时） |
| | `error` | `QString` | 错误描述（失败时） |

**错误情况**: 路径不存在、文件打开失败、文件超大

---

### 2.2 listDirectory

```cpp
ToolResult listDirectory(const QString &directoryPath, int maxEntries = 200);
```

列出指定目录下的文件和子目录。

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `directoryPath` | `const QString &` | — | 目录绝对路径 |
| `maxEntries` | `int` | `200` | 最大显示条目数 |

| 返回值 | 字段 | 类型 | 说明 |
|--------|------|------|------|
| `ToolResult` | `ok` | `bool` | 是否成功 |
| | `output` | `QString` | 格式化的目录内容 |
| | `error` | `QString` | 错误描述 |

---

### 2.3 saveTextFile

```cpp
ToolResult saveTextFile(const QString &filePath, const QString &content, bool allowOverwrite);
```

将文本内容保存到指定文件。

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `filePath` | `const QString &` | — | 目标文件路径 |
| `content` | `const QString &` | — | 要写入的文本内容 |
| `allowOverwrite` | `bool` | — | 是否允许覆盖已有文件 |

| 返回值 | 字段 | 类型 | 说明 |
|--------|------|------|------|
| `ToolResult` | `ok` | `bool` | 是否成功 |
| | `output` | `QString` | 成功消息或文件路径 |
| | `error` | `QString` | 错误描述 |

---

### 2.4 validateOpenPath

```cpp
ToolResult validateOpenPath(const QString &path);
```

校验路径是否存在且可被系统默认程序打开。

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `path` | `const QString &` | — | 文件或目录绝对路径 |

---

### 2.5 pathSummary

```cpp
QString pathSummary(const QString &path);
```

生成不含完整目录的路径摘要（用于日志脱敏）。

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `path` | `const QString &` | — | 完整路径 |

---

## 3. Agent Tool Registry 映射

在 `AgentToolRegistry::registerFileTools()` 中注册，AI 可通过 Function Calling 调用：

| Tool ID | 功能 | 风险等级 |
|---------|------|:---:|
| `file.read_text` | 读取文本文件 | Low |
| `file.list_directory` | 列出目录内容 | Low |
| `file.save_text` | 保存文本到文件 | Medium |
| `file.open_path` | 打开文件/文件夹 | Medium |

### Function Calling 参数示例

```json
// file.read_text
{"filePath": "/path/to/file.txt", "maxBytes": 1048576}

// file.list_directory
{"directoryPath": "/path/to/dir", "maxEntries": 200}

// file.save_text
{"filePath": "/path/to/output.txt", "content": "hello", "allowOverwrite": false}

// file.open_path
{"path": "/path/to/file_or_dir"}
```

---

## 4. Skill 集成

文件工具已注册为 4 个 Agent Skill（`.workbuddy/skills/`）：

| Skill 目录 | Skill 名 | 触发词 |
|-----------|---------|--------|
| `file-read-text/` | `file.read_text` | 读取文件, read file, 查看文件 |
| `file-list-directory/` | `file.list_directory` | 列出文件夹, list directory |
| `file-save-text/` | `file.save_text` | 保存文件, save file, 写入文件 |
| `file-open-path/` | `file.open_path` | 打开文件, open folder |

Agent 循环中会匹配用户输入关键词并注入对应 Skill 指令到 system prompt。

---

## 5. 后续可能复用的场景

| 场景 | 使用接口 | 说明 |
|------|---------|------|
| 批量文件处理命令 | `readTextFile` + `saveTextFile` | 技能链式调用实现自动批处理 |
| 工作区文件索引 | `listDirectory` | 项目文件探索 |
| MCP 外部工具 | 所有接口 | JSON-RPC 桥接给外部程序 |
| Agent 操作链 | 全部 Function Calling | 循环中自动读/写/列文件 |
| 日志导出 | `saveTextFile` | 导出 Agent 执行记录或日志 |

---

## 6. 变更记录

| 日期 | 版本 | 变更 |
|------|------|------|
| 2026-06-05 | 1.0 | 移除 `FileToolsDialog` UI 入口，保留 `FileInteractionService` API；注册为 Agent Skill；创建本文档 |
