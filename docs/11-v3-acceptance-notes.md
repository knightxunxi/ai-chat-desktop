# AI Chat Desktop V3 验收与复盘

日期：2026-05-22

## 1. V3 范围

V3 的目标是在 V2 可用聊天工具的基础上，补齐更接近企业桌面应用的安全性、可管理性、诊断能力和发布准备。

本轮 V3 覆盖：

- API Key 安全存储方案调研与设计。
- Windows Credential Manager 接入和旧 `QSettings` API Key 迁移。
- 会话列表顺序修复、重命名、搜索、导出和删除。
- DeepSeek、OpenAI 和自定义服务商预设。
- 可选模型参数配置，包括 temperature 和 max tokens。
- HTTP/网络错误分类提示。
- 失败后重试上一条用户消息。
- 应用内日志查看、刷新和打开日志目录。
- README、Windows 打包说明和 Release Notes 同步。

## 2. 自动化验证

建议在项目根目录执行：

```powershell
cmake --build build-qt
ctest --test-dir build-qt --output-on-failure
git diff --check
```

当前 Debug 构建目录 `build-qt` 验证结果：

- 构建通过。
- `15/15` 自动化测试通过。
- `git diff --check` 通过。
- 应用启动/关闭 smoke check 通过。

当前 Release 构建目录 `build-release-qt` 验证结果：

- Release 构建通过。
- `15/15` 自动化测试通过。
- `windeployqt` 发布依赖收集完成。
- 发布目录启动/关闭 smoke check 通过。
- 已生成 `release\AIChatDesktop-0.3.0-windows.zip`。

当前测试覆盖：

- `CoreModelSmokeTest`：默认配置、配置完整性、核心模型基础行为。
- `ProviderPresetTest`：服务商预设和自定义配置行为。
- `SessionSummaryListTest`：会话摘要置顶和列表顺序规则。
- `StreamParserTest`：SSE 流式响应解析。
- `OpenAICompatibleClientTest`：OpenAI 兼容请求体、模型参数和 HTTP 错误分类。
- `AppLoggerTest`：日志写入和敏感字段脱敏。
- `LogFileReaderTest`：日志最近内容读取。
- `ChatHistoryStorageTest`：SQLite 会话、消息存储、搜索和消息替换。
- `ChatSessionExporterTest`：当前会话 Markdown 导出。
- `ConfigStorageTest`：Windows 凭据存储抽象和旧配置迁移。
- `PromptTemplateStorageTest`：角色提示词模板 JSON 保存和加载。
- `SettingsDialogSmokeTest`：设置窗口、服务商预设和模型参数输入。
- `RolePromptDialogSmokeTest`：角色提示词模板选择和自定义切换。
- `MessageWidgetTest`：消息复制和 Markdown 展示行为。
- `StyleResourceTest`：Qt 样式资源加载。

## 3. V3 验收清单

| 验收项 | 状态 | 说明 |
| --- | --- | --- |
| CMake 成功构建 | 通过 | 使用 `cmake --build build-qt` 验证。 |
| 自动化测试全部通过 | 通过 | 当前 `15/15` 通过。 |
| API Key 不再写入普通配置 | 通过 | 使用 Windows Credential Manager 保存，新配置不写入 `api/apiKey`。 |
| 旧 API Key 配置迁移 | 通过 | `ConfigStorageTest` 覆盖旧 `QSettings` 迁移和清理。 |
| 会话列表切换不乱序 | 通过 | 切换会话不触发置顶，真正更新内容时才更新排序。 |
| 会话重命名 | 通过 | 当前会话可重命名，重启后标题保留。 |
| 会话搜索 | 通过 | 支持按标题和消息内容检索。 |
| 当前会话导出 | 通过 | 支持 Markdown 导出，包含角色、时间和消息内容。 |
| 服务商预设 | 通过 | 支持 DeepSeek、OpenAI 和自定义配置。 |
| 模型参数 | 通过 | temperature 和 max tokens 可选填写，请求体按需携带。 |
| 错误分类 | 通过 | HTTP 状态码和网络错误映射到更清晰的错误类型。 |
| 失败重试 | 通过 | 请求失败后可重试上一条用户消息。 |
| 应用内日志查看 | 通过 | 支持查看最近日志、刷新和打开日志目录。 |
| 敏感信息保护 | 通过 | 日志不记录 API Key、Bearer token、请求体或聊天正文。 |
| README 同步 | 通过 | 功能、测试和版本状态已更新。 |
| Windows 打包说明同步 | 通过 | 发布说明草稿更新为 V3 能力。 |
| Release 包准备 | 通过 | 已生成 `AIChatDesktop-0.3.0-windows.zip`。 |

## 4. 手工验收建议

使用有效 API Key 按以下顺序检查：

1. 启动 `build-qt\AIChatDesktop.exe`。
2. 打开设置，选择 DeepSeek 或 OpenAI 预设，确认 Base URL 和模型自动填充。
3. 保存 API Key，重启应用后确认配置仍可用。
4. 检查普通配置中不再出现 `api/apiKey`。
5. 新建会话，发送消息，确认 AI 回复流式展示。
6. 生成长回复时点击“停止”，确认可以取消并继续发送下一条。
7. 创建多个会话，切换后确认列表顺序保持稳定。
8. 重命名当前会话，重启后确认标题保留。
9. 使用左侧搜索框按标题和消息内容搜索会话。
10. 导出当前会话为 Markdown，确认内容完整且不包含 API Key。
11. 填写 temperature 或 max tokens 后发送消息，确认请求仍可用。
12. 临时填写错误 API Key，确认认证错误提示清楚且出现重试入口。
13. 打开“日志”窗口，确认能查看最近日志、刷新和打开目录。
14. 切换中文/英文界面，确认核心按钮文案刷新。
15. 关闭窗口后确认 `AIChatDesktop.exe` 进程退出。

日志通常位于：

```text
%APPDATA%\AIChatDesktop\AI Chat Desktop\ai-chat-desktop.log
```

## 5. V3 复盘

做得比较好的部分：

- 安全存储从需求、方案、抽象、实现到测试形成了完整闭环。
- V3 每个功能仍保持 feature branch 和 PR 合并节奏，`main` 维持稳定版本。
- 控制层继续承接业务流程，界面层主要负责展示和交互。
- 自动化测试从 V2 的 10 个扩展到 15 个，覆盖更多存储、配置、错误分类和日志读取行为。
- 文档在每个阶段同步更新，便于回顾开发流程和简历项目表达。

仍然存在的技术债：

- Markdown 展示仍是基础版本，没有语法高亮和复杂表格支持。
- UI 自动化测试仍缺失，当前主要依赖 smoke test 和人工验收。
- 角色提示词模板缺少导入、导出和分类管理。
- 会话搜索直接使用 SQLite LIKE，数据量很大时可能需要全文索引。
- 当前只面向 Windows，安全存储实现没有 macOS/Linux 适配。

## 6. 下一阶段候选方向

建议优先级：

- P0：V3 Release 包手工验收和 GitHub Release 发布。
- P1：Markdown 代码高亮和更丰富的代码块操作。
- P1：角色提示词模板导入导出。
- P2：基础 UI 自动化测试。
- P2：会话标签、收藏或批量整理。

## 7. 结论

V3 已完成预期目标：应用从 V2 的可用聊天工具，推进到具备安全凭据存储、会话管理增强、服务商预设、模型参数、错误诊断、失败重试和应用内日志查看的 Windows 桌面应用。

在个人项目和简历表述中，可以将本阶段总结为：

> 使用 C++17、Qt 6、CMake 和 SQLite 开发 Windows 桌面 AI 聊天应用，接入 Windows Credential Manager 保护 API Key，实践 Git/GitHub feature branch 与 Pull Request 流程，完成需求分析、架构设计、功能迭代、自动化测试、验收记录和发布准备。
