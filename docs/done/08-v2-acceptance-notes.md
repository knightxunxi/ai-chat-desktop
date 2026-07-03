# AI Chat Desktop V2 验收与复盘

日期：2026-05-22

## 1. V2 范围

V2 的目标是在 V1 基础聊天闭环之上，补齐架构、会话管理和常见聊天体验。

本轮 V2 覆盖：

- 抽出 `ApplicationController`，降低 `MainWindow` 业务复杂度。
- 完善多会话列表、切换、新建和删除。
- 支持停止生成。
- 支持消息复制。
- 支持 AI 消息基础 Markdown 渲染和代码块区分。
- 增加基础日志，不记录 API Key、请求体或聊天内容。
- 增加角色提示词模板。
- 更新 README、路线图和打包说明。

## 2. 自动化验证

建议在项目根目录执行：

```powershell
cmake --build build-qt
ctest --test-dir build-qt --output-on-failure
git diff --check
```

当前 Debug 构建目录 `build-qt` 验证结果：

- 构建通过。
- `10/10` 自动化测试通过。
- `git diff --check` 通过。

当前测试覆盖：

- `CoreModelSmokeTest`：默认配置、配置完整性、核心模型基础行为。
- `StreamParserTest`：SSE 流式响应解析。
- `OpenAICompatibleClientTest`：OpenAI 兼容请求体构建。
- `AppLoggerTest`：日志写入和敏感字段脱敏。
- `ChatHistoryStorageTest`：SQLite 会话和消息存储。
- `PromptTemplateStorageTest`：角色提示词模板 JSON 保存和加载。
- `SettingsDialogSmokeTest`：设置窗口基础行为。
- `RolePromptDialogSmokeTest`：角色提示词模板选择和自定义切换。
- `MessageWidgetTest`：消息复制和 Markdown 展示行为。
- `StyleResourceTest`：Qt 样式资源加载。

## 3. V2 验收清单

| 验收项 | 状态 | 说明 |
| --- | --- | --- |
| CMake 成功构建 | 通过 | 使用 `cmake --build build-qt` 验证。 |
| 自动化测试全部通过 | 通过 | 当前 `10/10` 通过。 |
| `ApplicationController` 接管业务流程 | 通过 | 主窗口通过信号槽与控制层通信。 |
| 多会话加载、切换、新建 | 通过 | 左侧会话列表已可用。 |
| 当前会话删除 | 通过 | 删除后 UI 和数据库状态同步。 |
| 停止生成 | 通过 | 生成中发送按钮切换为停止，可取消当前请求。 |
| 消息复制 | 通过 | 用户消息和 AI 消息均可复制原始文本。 |
| Markdown 基础展示 | 通过 | AI 消息支持标题、列表和基础格式。 |
| 代码块可读性 | 通过 | 代码块有浅灰背景、边框和等宽字体。 |
| 基础日志 | 通过 | 记录请求开始、完成、取消和失败，不记录敏感内容。 |
| 角色提示词模板 | 通过 | 支持选择、保存、删除模板，并应用到当前会话。 |
| README 和路线图同步 | 通过 | 本轮已更新项目状态。 |
| Windows 打包说明同步 | 通过 | 发布说明草稿更新为 V2 能力。 |

## 4. 手工验收建议

使用有效 API Key 按以下顺序检查：

1. 启动 `build-qt\AIChatDesktop.exe`。
2. 打开设置，确认 Base URL、模型、API Key 和语言保存正常。
3. 新建会话，发送消息，确认 AI 回复流式展示。
4. 生成长回复时点击“停止”，确认可以取消并继续发送下一条。
5. 创建多个会话，切换后确认历史内容恢复。
6. 删除当前会话，确认列表和聊天区状态正确。
7. 点击消息复制按钮，确认剪贴板内容为原始文本。
8. 让 AI 输出 Markdown 标题、列表和 C++ 代码块，确认显示可读。
9. 打开角色提示词，选择默认模板，应用后发送消息确认角色生效。
10. 新增、删除角色提示词模板，重启后确认模板仍保留。
11. 查看日志文件，确认有请求开始和完成记录，且没有 API Key。

日志通常位于：

```text
%APPDATA%\AIChatDesktop\AI Chat Desktop\ai-chat-desktop.log
```

## 5. V2 复盘

做得比较好的部分：

- 分支开发和 PR 合并流程稳定下来，`main` 保持为相对稳定版本。
- 每个功能都有明确任务边界，便于验收和回滚。
- 自动化测试从 V1 的 5 个扩展到 10 个，覆盖了更多非 UI 核心行为。
- 控制层抽出后，后续功能没有继续压到 `MainWindow`。
- 日志、模板、Markdown、复制等功能都保持了较小实现范围，没有过早复杂化。

仍然存在的技术债：

- API Key 仍使用普通本地配置保存，尚未接入系统凭据或加密存储。
- Markdown 仍是基础展示，没有语法高亮和复杂表格支持。
- UI 自动化测试仍缺失，当前主要依赖 smoke test 和人工验收。
- 多会话缺少重命名、搜索和导出。
- 日志只能写文件，应用内还没有日志查看入口。
- 角色提示词模板缺少导入、导出和分类管理。

## 6. V3 候选方向

建议优先级：

- P0：API Key 安全存储，减少敏感信息风险。
- P0：会话重命名、搜索和导出。
- P1：多服务商预设和模型参数配置。
- P1：更完整的错误分类和 UI 重试入口。
- P1：应用内日志查看入口。
- P2：Markdown 代码高亮和更丰富的消息操作。
- P2：基础 UI 自动化测试。

## 7. 结论

V2 已完成预期目标：应用从 V1 的基础聊天闭环，推进到具备控制层、多会话、停止生成、复制、Markdown、日志和角色模板的可用桌面聊天工具。

在个人项目和简历表述中，可以将本阶段总结为：

> 使用 C++、Qt 6 和 CMake 开发桌面 AI 聊天应用，实践 Git/GitHub feature branch 与 Pull Request 流程，完成需求拆分、架构重构、自动化测试、文档同步和迭代验收。
