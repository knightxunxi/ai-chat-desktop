# CodeXX 下一波开发规划

> 创建日期：2026-07-04
> 基线状态：本轮功能修复完成，`cmake --build build -j4` 通过，CTest 69/69 通过，Python sidecar 单测 28/28 通过。
> 目标：在 v1.0 稳定基线之后，按“可落地、可验证、可并行”的方式推进下一批能力。

---

## 1. 当前判断

当前主线不再是“补齐 Agent 基础能力”，而是进入“扩展能力 + 发布稳定性 + 长期架构边界”的阶段。

已完成的近期主线：

- Python sidecar 已可作为 AI 后端切换，并保留 C++ 直连回退。
- Agent 主循环、工具执行、MCP、记忆、Skills、Hooks、调度、UI 交互均已有可用闭环。
- 最近一轮修复补齐了配置保存反馈、sidecar 路径兜底、成功回复重新生成、子 Agent 客户端隔离和回归测试。

下一波不建议同时大改主控层和 UI。优先顺序应是：先发布稳定，再扩展外部能力，最后做架构级演进。

---

## 2. 下一波任务顺序

| 顺序 | 任务包 | 状态 | 是否可并行 | 目标 |
|------|--------|:---:|------------|------|
| N1 | 发布稳定与真实场景验收 | ✅ 2026-07-04 | 可与文档/小修并行 | 确认 v1.0 可演示、可打包、可回归 |
| N2 | Playwright 浏览器自动化 | ✅ 2026-07-04 | 与 N3 可并行，但不要同时改 Agent 主循环 | 让 Agent 具备真实网页操作能力 |
| N3 | 自更新系统 | 📌 2026-07-05 占位 | 与 N2 可并行 | 设置页面保留"检查更新"入口；真实 GitHub Release 接入后续推进 |
| N4 | 插件系统最小闭环 | ✅ 2026-07-04 | 依赖 N1 后做 | 为第三方/本地扩展工具预留边界 |
| N5 | 跨平台准备层 | ⬚ | 可与 N4 设计并行，代码实现需串行 | 把 Windows-only 能力抽象清楚 |
| N6 | 长期智能体增强 | ⬚ | 暂缓 | 双 Agent、角色系统、游戏自动化等长期方向 |

---

## 3. N1 发布稳定与真实场景验收

### 目标

把当前修复后的代码变成“可以放心交给别人试用”的稳定版本。

### 开发项

| 子项 | 内容 | 验收 |
|------|------|------|
| N1-1 | 补一份 v1.0 手工验收清单 | ✅ 已创建 `docs/v1.0-acceptance-checklist.md` |
| N1-2 | 打包产物二次检查 | ✅ 已检查并补充 `Qt6Concurrent.dll`，记录到 `docs/v1.0-packaging-report.md` |
| N1-3 | 真实 Agent 场景脚本 | ✅ 已创建 `docs/v1.0-agent-scenarios.md` (8 个场景) |
| N1-4 | 文档状态收敛 | ✅ 已集成到 DOCUMENT_INDEX.md |

### 并行建议

可与 N3 的文档/设计并行；不要和大规模 UI 改造同时做。

---

## 4. N2 Playwright 浏览器自动化

### 目标

通过 Python sidecar 承载浏览器自动化能力，C++ 只负责工具注册、权限边界和结果展示。

### 推荐边界

```text
AgentToolRegistry
  -> browser.open / browser.click / browser.extract / browser.screenshot
  -> Python sidecar browser.* method
  -> Playwright
  -> 结构化 observation 返回 C++
```

Python 能力层执行浏览器动作，但不接管 Agent 主循环，也不直接修改本机文件。

### 任务拆分

| 子项 | 内容 | 验收 |
|------|------|------|
| N2-1 | sidecar 增加 `browser.ping` 和依赖检测 | ✅ 已实现 `_check_playwright_installed()` |
| N2-2 | 增加 `browser.open` / `browser.extract_text` | ✅ 已实现，C++ 工具已注册 |
| N2-3 | 增加 `browser.screenshot` | ✅ 已实现，截图路径返回 |
| N2-4 | C++ 注册浏览器工具 | ✅ `browser.open` / `browser.extract_text` / `browser.screenshot` |
| N2-5 | 测试 | ✅ 8 个 Python 浏览器单测（含无依赖降级、URL/输出目录边界）+ C++ 69/69 零回归 |

### 参考

- Playwright Python：<https://playwright.dev/python/docs/intro>
- Playwright Page API：<https://playwright.dev/python/docs/api/class-page>

---

## 5. N3 自更新系统

### 目标

v1.0 先保留“检查更新”入口作为占位，不访问占位仓库、不自动下载、不自动替换。真实发布仓库确认后，再接入“检查更新 + 提示下载 + 打开 Release 页面/下载 zip”的安全版本。

### 任务拆分

| 子项 | 内容 | 验收 |
|------|------|------|
| N3-1 | 版本元数据模型 | 📌 `UpdateInfo` 保留，新增 `placeholderMessage` |
| N3-2 | GitHub Releases 查询 | ⬚ 暂缓，避免访问占位仓库 |
| N3-3 | UI 提示 | 📌 设置页面"检查更新"按钮保留，占位弹窗提示暂未启用 |
| N3-4 | 安全边界 | ✅ 不自动覆盖、不自动下载、不自动执行 |
| N3-5 | 后续增强预留 | 📌 版本比较和 GitHub 响应解析代码保留，真实仓库接入后启用 |

### 参考

- GitHub Releases API：<https://docs.github.com/rest/releases/releases>
- GitHub Release Assets API：<https://docs.github.com/rest/releases/assets>
- WinSparkle：<https://winsparkle.org/>

---

## 6. N4 插件系统最小闭环

### 目标

先做“插件描述 + 插件发现 + 禁用/启用 + 工具注册”的最小闭环，不急于支持复杂 UI 插件。

### 推荐边界

```text
plugins/
  plugin.json
  plugin.dll

PluginManager
  -> 读取 manifest
  -> QPluginLoader 加载 DLL
  -> 转换为 AgentToolDefinition
```

### 任务拆分

| 子项 | 内容 | 验收 |
|------|------|------|
| N4-1 | 插件 manifest 格式 | ✅ plugin.json (id / name / version / tools / risk / schema) |
| N4-2 | PluginManager | ✅ 扫描应用目录/项目目录插件 → 解析 manifest → QPluginLoader → 工具提取 |
| N4-3 | 工具注册桥接 | ✅ `registerPluginTools()` 方法 + AgentOrchestrator::toolRegistry() 集成 |
| N4-4 | 插件 UI | 📌 PluginManager 已暴露 API，ToolsDialog 接入留待下轮 |
| N4-5 | 示例插件 | ✅ `plugins/examples/ExamplePlugin` — `plugin.hello_world` 工具 |
| N5 | **跨平台准备层** | ✅ 2026-07-04 | 3-5 天 | 平台抽象 |

### 参考

- Qt QPluginLoader：<https://doc.qt.io/qt-6/qpluginloader.html>

---

## 7. N5 跨平台准备层

### 目标

不立即承诺 macOS/Linux 完整可用，先把 Windows-only 能力集中抽象，避免继续散落在上层业务逻辑。

### 任务拆分

| 子项 | 内容 | 验收 |
|------|------|------|
| N5-1 | PlatformServices 接口 | ✅ 纯虚接口：Credential / OCR / Input / WindowDetect / ForegroundGuard |
| N5-2 | WindowsPlatformServices | ✅ Win32 Credential Manager / EnumWindows / SendInput 包装 |
| N5-3 | UnsupportedPlatformServices | ✅ 非 Windows 返回明确不可用错误 |
| N5-4 | CMake 平台开关 | ✅ WIN32 条件链接 + 独立子库 |

### 并行建议

N5 设计可与 N4 并行；代码改造不建议和 N2 浏览器工具同时动工具注册表。

---

## 8. N6 长期智能体增强

这些方向先保持规划，不进入下一波主线：

| 方向 | 暂缓原因 | 进入条件 |
|------|----------|----------|
| 双 Agent 互审 | 会放大模型调用成本和状态复杂度 | 插件系统和审查结构化稳定后 |
| 角色扮演系统 | 产品定位会偏离编程助手 | 主线开发助手体验稳定后 |
| 游戏自动化 | 需要大量场景适配和图像状态机 | 浏览器/桌面自动化工具稳定后 |
| 完整跨平台 | Windows-only 能力太多 | N5 抽象完成后再评估 |

---

## 9. 推荐并行矩阵

| 组合 | 建议 | 原因 |
|------|:---:|------|
| N1 发布验收 + N3 自更新设计 | ✅ | 一个偏验证，一个偏网络/UI，冲突小 |
| N2 Playwright + N3 自更新 | ✅ | 分属 Python sidecar 和 Release 查询，可并行 |
| N2 Playwright + N4 插件系统 | ⚠️ | 都会接触工具注册和 schema，需拆清文件 |
| N4 插件系统 + N5 平台抽象 | ⚠️ | 设计可并行，代码建议串行 |
| N2 Playwright + Agent 主循环重构 | ❌ | 一个新增能力，一个动主流程，回归面过大 |
| N3 自更新 + 打包 CI | ✅ | 发布链路互补 |

---

## 10. 下一步建议

优先做 N1。N1 完成后可开两个并行分支：

1. `feature/browser-automation-sidecar`：推进 N2。
2. `feature/self-update`：推进 N3。

插件系统和跨平台准备属于第二波，不建议抢在浏览器自动化和自更新前面。
