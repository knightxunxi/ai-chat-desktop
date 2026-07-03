# AI 聊天桌面应用 V1 验收记录

日期：2026-05-20

## 1. 验收范围

本记录对应 `docs/01-requirements.md` 中 V1 验收标准，以及 `docs/03-task-breakdown.md` 阶段 8 的 TASK-018。

当前验收重点：

- 项目能否构建。
- 非 UI 核心逻辑测试是否通过。
- 配置、会话、角色提示词、流式解析和聊天记录的实现是否覆盖 V1 范围。
- 真实 API 和人工 UI 操作还需要在有 API Key 的环境中补做最终手工验收。

## 2. 自动化验证

建议在项目根目录执行：

```powershell
cmake --build build-qt
ctest --test-dir build-qt --output-on-failure
git diff --check
```

当前自动化测试覆盖：

- `CoreModelSmokeTest`：默认配置、配置完整性、核心模型基础行为。
- `StreamParserTest`：SSE 文本片段、半包数据、`[DONE]`、末尾无换行残留行。
- `OpenAICompatibleClientTest`：请求体构建、角色提示词注入、空 AI 占位消息过滤。
- `ChatHistoryStorageTest`：SQLite 初始化、会话保存、消息保存、角色提示词恢复、清空会话。
- `SettingsDialogSmokeTest`：设置窗口配置读写基础行为。

本轮结果：

- Debug 构建目录 `build-qt`：构建通过，5 个测试全部通过。
- Release 构建目录 `build-release-qt`：构建通过，5 个测试全部通过。
- 发布目录 `release/AIChatDesktop`：已通过 `windeployqt` 收集依赖。
- GUI 启动验收：本轮未完成，需要人工双击发布目录中的 exe 确认。

## 3. V1 验收清单

| 验收项 | 状态 | 说明 |
| --- | --- | --- |
| CMake 成功构建 | 通过 | 使用 `cmake --build build-qt` 验证。 |
| 应用可以启动并显示主窗口 | 待人工确认 | 已生成发布目录，需手工启动 GUI 确认。 |
| 设置 API Key、Base URL、模型名称 | 通过 | `SettingsDialog` 已实现并有 smoke test。 |
| 重启后读取配置 | 通过 | `ConfigStorage` 使用 `QSettings` 保存配置。 |
| 发送消息到 OpenAI 兼容接口 | 待真实 API 验证 | 代码已接入 `OpenAICompatibleClient`，需有效 API Key。 |
| AI 回复显示在聊天窗口 | 待真实 API 验证 | UI 已接收流式片段并更新最后一条 AI 消息。 |
| AI 回复流式逐步展示 | 待真实 API 验证 | `StreamParser` 已有自动化测试，真实网络流需手工确认。 |
| 连续对话携带历史上下文 | 通过 | 请求体会包含当前会话非空用户/助手消息。 |
| 当前会话角色提示词生效 | 通过 | 请求体测试确认 `system` 消息位于历史消息前。 |
| 中文/英文界面语言切换 | 待人工确认 | 配置和 UI 文案刷新已实现，需点击确认。 |
| 聊天记录保存和恢复 | 通过 | `ChatHistoryStorageTest` 覆盖会话、消息、角色提示词。 |
| 配置缺失提示 | 通过 | 发送前校验 `AppConfig::isComplete()` 并提示。 |
| 网络/API 错误提示 | 部分通过 | 客户端会解析 HTTP/API 错误；真实错误场景需联网确认。 |
| 等待 AI 回复期间界面不卡死 | 待人工确认 | 网络请求异步执行，仍需手工体验确认。 |

## 4. 已知限制

- 当前 V1 采用 `MainWindow` 直接串联 UI、存储和 API 客户端，没有单独落地 `ApplicationController`。
- 当前已支持基础多会话列表和切换，但尚未支持重命名、删除、搜索等完整会话管理。
- API Key 使用 `QSettings` 普通本地保存，V2/V3 应评估系统凭据或加密存储。
- 尚未执行带有效 DeepSeek/OpenAI 兼容 API Key 的端到端验收。

## 5. 最终手工验收建议

在准备发布前，使用有效 API Key 按以下顺序手工检查：

1. 启动 `AIChatDesktop.exe`。
2. 打开设置，保存 Base URL、模型名称、API Key 和语言。
3. 关闭并重新启动，确认配置保留。
4. 发送第一条消息，确认用户消息、AI 占位和流式回复。
5. 设置角色提示词，再发送消息，确认回复风格受影响。
6. 连续发送第二条消息，确认上下文连续。
7. 新建会话，确认界面清空且旧会话已保存。
8. 使用错误 API Key 或错误模型名，确认错误提示清晰。
9. 关闭并重新打开，确认最近会话和角色提示词恢复。
