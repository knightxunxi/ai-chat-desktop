# V5 验收记录

本文档记录 AI Chat Desktop V5 阶段的完成情况、验证结果和后续建议。

执行状态：V5-TASK-001 至 V5-TASK-008 已完成。

## 1. 完成范围

V5 完成了两类主要能力：

- 本地小工具系统：提供统一工具接口、工具窗口和首批内置文本处理工具。
- 会话组织增强：提供收藏、归档和筛选能力，并保持会话排序稳定。

## 2. 任务完成情况

| 任务 | 状态 | 说明 |
| --- | --- | --- |
| V5-TASK-001 本地工具抽象 | 完成 | 新增 `LocalTool` 和 `ToolResult` |
| V5-TASK-002 JSON 格式化和压缩 | 完成 | 新增 JSON 格式化、JSON 压缩工具 |
| V5-TASK-003 工具窗口和主窗口入口 | 完成 | 新增 `ToolsDialog` 和主窗口“工具”入口 |
| V5-TASK-004 工具窗口 smoke test | 完成 | 新增 `ToolsDialogSmokeTest` |
| V5-TASK-005 Markdown 和文本清理工具 | 完成 | 新增 Markdown 整理和普通文本清理工具 |
| V5-TASK-006 会话组织增强设计 | 完成 | 新增会话组织增强设计文档 |
| V5-TASK-007 会话组织第一版实现 | 完成 | 新增收藏、归档、筛选和旧库补列 |
| V5-TASK-008 V5 验收和项目展示更新 | 完成 | 更新 README、learn 文档和本验收记录 |

## 3. 新增代码能力

### 3.1 本地工具系统

新增目录：

```text
src/tools/
```

核心设计：

- `ToolResult`：统一表示工具成功输出和失败错误。
- `LocalTool`：本地工具抽象接口。
- `JsonFormatTool`：JSON 缩进格式化。
- `JsonCompactTool`：JSON 单行压缩。
- `MarkdownCleanupTool`：清理 Markdown 代码块外部空白。
- `TextCleanupTool`：统一普通文本换行和空白。

工具窗口：

- 支持工具选择。
- 支持输入区和输出区。
- 支持运行、复制输出、插入聊天输入框。
- 插入输出不会自动发送，仍由用户确认。

### 3.2 会话组织增强

新增模型字段：

```cpp
bool isFavorite = false;
bool isArchived = false;
```

新增筛选类型：

```cpp
enum class SessionListFilter {
    Active,
    Favorite,
    Archived
};
```

存储增强：

- `sessions` 表新增 `is_favorite`。
- `sessions` 表新增 `is_archived`。
- 旧数据库启动时自动补列。
- 默认列表只显示未归档会话。
- 收藏筛选只显示未归档收藏会话。
- 归档筛选只显示归档会话。
- 搜索结果遵守当前筛选条件。

UI 增强：

- 侧边栏新增“收藏/取消收藏”。
- 侧边栏新增“归档/取消归档”。
- 侧边栏新增“全部/收藏/归档”筛选。
- 收藏会话在列表标题前显示 `[*]` 标记。

## 4. 验证结果

已执行：

```powershell
cmake --build build-qt
ctest --test-dir build-qt --output-on-failure
git diff --check
```

结果：

- 构建通过。
- `ctest` 共 20 个测试，全部通过。
- `git diff --check` 通过。

## 5. 新增或更新测试

V5 新增测试：

- `LocalToolContractTest`
- `JsonToolsTest`
- `MarkdownTextToolsTest`
- `ToolsDialogSmokeTest`

V5 更新测试：

- `ChatHistoryStorageTest`

新增覆盖点：

- 工具接口契约。
- JSON 合法/非法输入。
- Markdown 代码块保护。
- 文本换行和空白清理。
- 工具窗口基础交互。
- 收藏和归档状态保存。
- 收藏和归档筛选。
- 搜索和筛选组合。
- 旧 SQLite 数据库自动补列。
- 收藏/归档不改变 `updatedAt`。

## 6. 手工验证建议

合并前建议手工检查：

- 主窗口顶部“工具”按钮可以打开工具窗口。
- JSON 格式化、压缩、Markdown 整理、文本清理可用。
- 工具输出可以复制。
- 工具输出可以插入聊天输入框，且不会自动发送。
- 收藏按钮可切换当前会话收藏状态。
- 归档按钮可切换当前会话归档状态。
- 默认、收藏、归档筛选显示正确。
- 搜索框和筛选条件可以组合使用。
- 重启应用后收藏/归档状态仍保留。

## 7. 风险和遗留项

当前仍保留的限制：

- 收藏不会置顶，仍按 `updatedAt DESC` 排序。
- 标签暂未实现。
- SQLite FTS 全文搜索暂未实现。
- 工具系统仅支持内置 C++ 工具，不支持插件或脚本。
- Agent 和电脑交互能力只在后续规划中记录，未进入 V5 实现。

这些限制是有意保留的，目的是让 V5 保持边界清晰，避免一次引入过多高风险能力。

## 8. 后续建议

V5 之后可以进入两条路线：

- V6：受控本地文件/系统交互工具。
- 会话增强后续版：标签、全文搜索、批量操作。

如果优先考虑简历展示，建议先补充：

- GitHub PR 截图或 Actions 通过记录。
- Release 构建产物。
- 简历项目描述中的 V5 能力更新。
