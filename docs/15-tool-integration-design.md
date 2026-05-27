# 小工具集成方案

本文档对应 V4-TASK-006，用于规划 AI Chat Desktop 后续的小工具集成方式。目标是先建立稳定边界，再逐步增加工具，避免把工具逻辑直接堆进 `MainWindow`。

## 1. 目标

小工具系统解决两类需求：

- 本地辅助工具：不调用 AI，只处理用户输入，例如 JSON 格式化、Markdown 格式化、文本清理。
- 聊天辅助入口：把工具输出作为用户可复制或可发送给 AI 的内容。

V4 阶段只做设计，不直接引入复杂插件系统。

## 2. 非目标

当前阶段不做：

- 插件市场。
- 第三方脚本运行。
- 网络工具调用。
- AI 自动调用工具。
- 用户自定义代码执行。
- 跨进程工具沙箱。

原因：这些能力涉及权限、安全、依赖管理和复杂 UI，不适合作为当前阶段的第一步。

## 3. 推荐架构

建议新增目录：

```text
src/tools/
  LocalTool.h
  ToolResult.h
  JsonFormatTool.h/.cpp
  MarkdownFormatTool.h/.cpp
```

建议新增 UI：

```text
src/ui/
  ToolsDialog.h/.cpp
```

整体调用关系：

```text
MainWindow
  ↓
ToolsDialog
  ↓
LocalTool interface
  ↓
Concrete tools
```

工具逻辑不直接依赖 `MainWindow`、`ApplicationController` 或聊天存储。

## 4. 工具抽象

建议定义本地工具接口：

```cpp
struct ToolResult {
    bool ok = false;
    QString output;
    QString error;
};

class LocalTool
{
public:
    virtual ~LocalTool() = default;

    virtual QString id() const = 0;
    virtual QString displayName(AppLanguage language) const = 0;
    virtual QString description(AppLanguage language) const = 0;
    virtual ToolResult run(const QString &input) const = 0;
};
```

设计原则：

- 工具输入输出都是文本，先保持简单。
- 工具返回结构化结果，UI 不解析异常字符串。
- 工具显示文本支持中英文。
- 工具不直接弹窗，不直接访问剪贴板。

## 5. 首批工具建议

### 5.1 JSON 格式化

输入：

```json
{"name":"test","items":[1,2,3]}
```

输出：

```json
{
    "name": "test",
    "items": [
        1,
        2,
        3
    ]
}
```

实现方式：

- 使用 `QJsonDocument::fromJson` 解析。
- 成功后使用 `toJson(QJsonDocument::Indented)` 输出。
- 失败时返回 JSON parse error。

验收：

- 合法 JSON 可以格式化。
- 非法 JSON 有清晰错误。
- 不修改用户原始输入，除非用户复制输出。

### 5.2 JSON 压缩

输入格式化 JSON，输出 compact JSON。

实现方式：

- 同样使用 `QJsonDocument`。
- 成功后使用 `toJson(QJsonDocument::Compact)`。

### 5.3 Markdown 简单整理

初期只做低风险整理：

- 统一行尾空格。
- 压缩过多空行。
- 保留代码块内容。

不建议第一版做复杂 Markdown AST 解析。

### 5.4 文本清理

可做：

- 去除首尾空白。
- 将 Windows 换行统一为 `\n`。
- 去除重复空行。

## 6. 工具窗口 UI

建议 `ToolsDialog` 使用三栏或上下结构：

```text
工具选择
输入文本
输出文本
按钮：运行 / 复制输出 / 插入到输入框 / 关闭
```

首版建议：

- 从主窗口顶部增加“工具”按钮。
- 工具窗口不阻塞当前会话状态。
- 工具输出默认不自动发送给 AI。
- 提供“复制输出”即可。

后续可以增加：

- “发送到当前聊天输入框”。
- “作为新消息发送”。
- 工具历史记录。

## 7. 与聊天流程的边界

小工具系统不应该直接调用 `ApplicationController::sendMessage()`。

原因：

- 用户需要确认工具输出是否正确。
- 工具失败不应影响聊天状态。
- 工具和聊天应该可以独立测试。

如果后续要支持“发送到输入框”，建议由 `MainWindow` 接收工具输出，再填入 `m_messageInput`，仍然由用户点击发送。

## 8. 测试策略

工具逻辑测试：

- `JsonFormatToolTest`
- `MarkdownFormatToolTest`

UI smoke test：

- 工具窗口可以打开。
- 切换工具后说明文本更新。
- 输入合法 JSON 后输出格式化结果。

不建议第一版用 UI 测试覆盖所有文本细节，核心转换逻辑应在工具单元测试中覆盖。

## 9. 安全边界

首版工具只运行内置 C++ 逻辑，不执行用户脚本。

后续如果考虑插件或脚本，需要额外设计：

- 权限模型。
- 文件访问限制。
- 网络访问限制。
- 崩溃隔离。
- 插件签名或来源校验。

当前项目不进入这些复杂范围。

## 10. 推荐落地顺序

```text
1. 新增 LocalTool / ToolResult 抽象。
2. 实现 JsonFormatTool。
3. 增加 JsonFormatToolTest。
4. 新增 ToolsDialog。
5. 主窗口增加“工具”入口。
6. 增加工具窗口 smoke test。
7. 再扩展 Markdown/Text 工具。
```

## 11. 简历表达

如果后续实现，可以在简历中描述为：

> 设计本地工具扩展边界，抽象 LocalTool 接口，将 JSON 格式化等工具逻辑与主聊天流程解耦，支持后续扩展更多文本处理工具。

这个表达比“做了几个小工具”更有工程含义。
