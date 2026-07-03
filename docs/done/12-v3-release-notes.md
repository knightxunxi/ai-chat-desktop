# AI Chat Desktop 0.3.0 Release Notes

发布日期：2026-05-22

## 1. 版本定位

`0.3.0` 是 AI Chat Desktop 的 V3 阶段版本，重点补齐安全存储、会话管理、服务商配置、错误诊断和应用内日志查看能力。

## 2. 新增能力

- API Key 使用 Windows Credential Manager 保存。
- 旧 `QSettings` API Key 自动迁移到系统凭据存储。
- 会话列表切换不再导致顺序变乱。
- 支持会话重命名、搜索、导出和删除。
- 支持 DeepSeek、OpenAI 和自定义服务商预设。
- 支持可选模型参数：temperature、max tokens。
- 请求失败时展示网络、认证、额度、模型、服务端等分类提示。
- 请求失败后支持重试上一条用户消息。
- 应用内日志查看窗口支持刷新和打开日志目录。
- 聊天导出时间使用 UTC+8 北京时间。
- 关闭主窗口时主动取消请求并退出进程。

## 3. 安全与隐私

- API Key 不再写入普通本地配置。
- 日志不记录 API Key、Bearer token、请求体或聊天正文。
- Markdown 导出不包含 API Key。
- 发布包不携带用户本机凭据，每台机器需要单独配置 API Key。

## 4. 验证结果

当前 V3 验证结果：

- Debug 构建通过。
- `15/15` 自动化测试通过。
- `git diff --check` 通过。
- 应用启动/关闭 smoke check 通过。
- Release 构建。
- Release `15/15` 自动化测试通过。
- `windeployqt` 依赖收集完成。
- 发布目录启动/关闭 smoke check 通过。
- 已生成 `release\AIChatDesktop-0.3.0-windows.zip`。

## 5. 已知限制

- 当前版本只面向 Windows。
- Markdown 展示仍是基础版本，复杂代码高亮暂未支持。
- UI 自动化测试暂未接入。
- 角色提示词模板暂不支持导入导出。
- 会话搜索暂未使用全文索引，大量历史记录场景下后续可优化。

## 6. 升级提示

从 V2 或更早版本升级时：

- 首次启动会尝试将旧配置中的 API Key 迁移到 Windows Credential Manager。
- 如果迁移失败，用户可以在设置窗口重新保存 API Key。
- 聊天记录仍保存在本机 SQLite 数据库中。
