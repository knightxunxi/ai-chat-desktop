# 会话组织增强设计

本文档对应 V5-TASK-006，用于规划会话标签、收藏、归档和搜索增强。目标是在不破坏现有会话顺序和历史存储的前提下，逐步提高多会话管理能力。

执行状态：设计完成，第一版“收藏 + 归档 + 筛选”已实现。

## 1. 当前状态

当前会话相关能力：

- `ChatSession` 保存会话 ID、标题、角色提示词、创建时间、更新时间和消息列表。
- `ChatHistoryStorage` 使用 SQLite 保存 `sessions` 和 `messages` 两张表。
- 会话列表按 `updated_at DESC` 排序。
- 搜索使用标题和消息内容 `LIKE` 匹配。
- `SessionSummaryList::upsert(..., moveToTop)` 控制内存列表更新时是否移动到顶部。

当前已修复过的关键问题：

- 点击会话不应导致会话顺序变乱。
- 会话列表当前选中项应与当前会话 ID 保持一致。

因此后续实现会话组织功能时，必须把“排序稳定”作为约束。

## 2. 目标

V5 会话组织增强的目标：

- 让用户能快速标记重要会话。
- 让不常用会话可以归档，减少侧边栏干扰。
- 保持当前列表排序逻辑清晰、可预测。
- 支持未来扩展标签和全文搜索。
- 存储结构兼容旧数据库。

## 3. 非目标

V5 暂不做：

- 云同步。
- 多账号会话隔离。
- 复杂标签颜色系统。
- 拖拽排序。
- 会话文件夹树。
- SQLite FTS 全文搜索实现。

这些功能会增加存储迁移、UI 状态和测试复杂度，不适合作为第一版。

## 4. 推荐优先级

建议优先级：

```text
1. 收藏
2. 归档
3. 标签
4. SQLite FTS 全文搜索
```

原因：

- 收藏和归档只需要给会话增加少量状态字段，适合第一版。
- 标签需要额外的数据结构和筛选 UI，复杂度中等。
- FTS 涉及虚拟表、同步策略和迁移成本，应等基础组织功能稳定后再做。

## 5. 数据结构设计

### 5.1 第一版字段

建议先在 `ChatSession` 增加：

```cpp
bool isFavorite = false;
bool isArchived = false;
```

对应 SQLite `sessions` 表新增字段：

```sql
is_favorite INTEGER NOT NULL DEFAULT 0
is_archived INTEGER NOT NULL DEFAULT 0
```

兼容策略：

- `initialize()` 中使用 `ALTER TABLE` 尝试补列。
- 如果列已存在，不视为错误。
- 读取旧数据库时默认值为 `false`。

### 5.2 标签字段

标签建议暂不在第一版实现。后续可选两种方案：

方案 A：简单 JSON 字段

```sql
tags TEXT NOT NULL DEFAULT '[]'
```

优点：

- 实现快。
- 不需要额外表。

缺点：

- 查询和去重不方便。
- 标签统计能力弱。

方案 B：关系表

```sql
session_tags (
    session_id TEXT NOT NULL,
    tag TEXT NOT NULL,
    PRIMARY KEY(session_id, tag)
)
```

优点：

- 查询、过滤、统计更清晰。
- 后续支持标签列表更方便。

缺点：

- 实现和测试成本更高。

建议：如果 V5 要做标签，优先使用关系表。

## 6. 排序规则

必须保持规则简单：

- 默认列表：未归档会话，按 `updated_at DESC`。
- 收藏筛选：只显示收藏会话，仍按 `updated_at DESC`。
- 归档筛选：只显示归档会话，仍按 `updated_at DESC`。
- 搜索结果：在当前筛选范围内搜索，仍按 `updated_at DESC`。

不建议第一版做“收藏置顶”。

原因：

- 收藏置顶会引入第二排序维度。
- 用户点击、收藏、搜索时更容易出现“列表跳动”。
- 当前项目已经关注过会话顺序稳定问题，应先保持排序规则单一。

## 7. UI 设计

### 7.1 侧边栏筛选入口

建议在搜索框附近增加筛选控件：

```text
全部 / 收藏 / 归档
```

第一版可以使用：

- `QComboBox`
- 或 3 个小按钮

考虑现有 UI 简洁性，建议先用 `QComboBox`。

### 7.2 会话操作按钮

当前侧边栏已有：

- 新建会话
- 重命名
- 导出
- 删除会话

建议第一版新增：

- 收藏/取消收藏
- 归档/取消归档

如果按钮过多，可以后续合并为菜单；第一版为了实现清晰，可以先使用按钮。

### 7.3 会话列表展示

建议在标题前增加简单标记：

```text
★ 项目讨论
```

归档会话只在归档筛选中显示，不在默认列表显示。

## 8. 控制层设计

`ApplicationController` 建议新增：

```cpp
enum class SessionListFilter {
    Active,
    Favorite,
    Archived
};

void setSessionListFilter(SessionListFilter filter);
void toggleCurrentSessionFavorite();
void toggleCurrentSessionArchived();
SessionListFilter sessionListFilter() const;
```

行为要求：

- 切换筛选条件只刷新列表，不修改当前会话内容。
- 收藏当前会话不应改变 `updatedAt`。
- 归档当前会话不应改变 `updatedAt`。
- 点击会话只切换当前会话，不改变列表排序。
- 发送消息、重命名等内容变化才允许更新 `updatedAt`。

## 9. 存储层设计

`ChatHistoryStorage` 建议新增：

```cpp
QVector<ChatSession> loadSessionSummaries(SessionListFilter filter, QString *errorMessage = nullptr) const;
QVector<ChatSession> searchSessionSummaries(const QString &query, SessionListFilter filter, QString *errorMessage = nullptr) const;
bool setSessionFavorite(const QString &sessionId, bool favorite, QString *errorMessage = nullptr);
bool setSessionArchived(const QString &sessionId, bool archived, QString *errorMessage = nullptr);
```

实现要求：

- `saveSession()` 需要写入 `is_favorite` 和 `is_archived`。
- `readSessionSummaries()` 需要读回这两个字段。
- 筛选条件应进入 SQL `WHERE`，不要在 UI 层过滤。
- 旧数据库迁移必须自动完成。

## 10. 测试策略

### 10.1 存储测试

需要覆盖：

- 新数据库建表后包含组织字段。
- 旧数据库补列后读取默认值正确。
- 收藏状态保存后重启仍保留。
- 归档状态保存后重启仍保留。
- 默认列表不显示归档会话。
- 归档筛选只显示归档会话。
- 搜索结果遵守当前筛选。

### 10.2 控制层测试

需要覆盖：

- 切换筛选不改变当前会话。
- 收藏不改变会话排序。
- 归档当前会话后列表刷新稳定。
- 点击会话不改变列表顺序。

### 10.3 UI 手工验证

需要验证：

- 收藏按钮文案会随状态切换。
- 归档按钮文案会随状态切换。
- 默认筛选、收藏筛选、归档筛选显示正确。
- 当前会话高亮不丢失。
- 搜索框和筛选条件可以组合使用。

## 11. 第一版推荐实现范围

V5-TASK-007 建议只实现：

- `ChatSession::isFavorite`
- `ChatSession::isArchived`
- SQLite 字段迁移
- 默认/收藏/归档筛选
- 收藏当前会话
- 归档当前会话
- 存储测试和控制层测试

暂不实现：

- 标签
- FTS
- 收藏置顶
- 拖拽排序
- 批量操作

## 12. 风险点

- 数据库迁移如果处理不当，会导致旧用户历史读取失败。
- 归档当前会话后，如果默认列表隐藏归档项，需要明确当前会话是否继续显示。
- 收藏/归档不应刷新 `updatedAt`，否则会导致列表顺序变化。
- 搜索和筛选组合时，SQL 条件容易遗漏。
- UI 按钮过多会挤压侧边栏空间，后续可能需要合并为菜单。

## 13. 验收标准

设计进入实现前应满足：

- 数据字段明确。
- 列表排序规则明确。
- 筛选规则明确。
- UI 入口明确。
- 兼容旧数据库策略明确。
- 测试范围明确。

第一版实现完成时应满足：

- 收藏和归档状态可保存、读取、重启保留。
- 默认列表不显示归档会话。
- 收藏和归档筛选可用。
- 点击会话不会改变会话顺序。
- 当前会话高亮稳定。
- 本地 `ctest` 和 GitHub Actions 通过。
