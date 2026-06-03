# V9 验收记录：受控命令执行 MVP 第一版

## 1. 版本目标

V9 的目标是让 Agent 可以执行有限、白名单、可审计、可超时的命令。第一版只开放固定命令模板，不开放任意 PowerShell、CMD 或模型拼接参数。

本阶段仍然不包含：

- 任意 PowerShell/CMD 字符串执行。
- `git add`、`git commit`、`git push`。
- 删除、格式化、注册表修改、系统服务修改。
- 键盘鼠标模拟。
- 后台无人值守长期任务。

## 2. 完成内容

已完成：

- 新增 [V9 命令执行安全设计](32-v9-command-execution-security.md)。
- 新增 `CommandPolicy`。
- 新增 `CommandRunner`。
- 将 `command.*` 工具加入 `AgentToolRegistry`。
- `AgentPlanExecutor` 支持项目目录上下文。
- `AgentPlanDialog` 支持把项目目录传给命令工具，并在命令步骤详情中展示项目目录。
- `MainWindow` 打开计划窗口时传入当前进程工作目录作为项目目录。
- Function Calling schema 中包含 `command.*` 工具。

## 3. 已开放命令模板

| 工具 ID | 能力 | 风险 |
| --- | --- | --- |
| `command.git_status` | `git status --short --branch` | low |
| `command.git_diff_check` | `git diff --check` | low |
| `command.git_diff_stat` | `git diff --stat` | low |
| `command.cmake_build` | `cmake --build build-qt` | medium |
| `command.ctest` | `ctest --test-dir build-qt --output-on-failure` | medium |
| `command.list_project_files` | 内部 Qt 枚举项目根目录 | low |

所有命令工具第一版都不接受模型提供的任意参数，`parameters` 必须为空对象。

## 4. 安全边界

`CommandPolicy` 边界：

- 只允许固定模板 ID。
- 拒绝未知命令。
- 拒绝空工作目录、磁盘根目录、用户主目录根部和系统关键目录。
- 固定命令参数中不允许 shell 元字符。
- 禁止 `powershell`、`cmd`、`Remove-Item` 等危险程序进入模板。

`CommandRunner` 边界：

- 使用 `QProcess` 的程序和参数数组，不走 shell 字符串。
- 设置命令超时。
- 非零退出码返回失败摘要。
- 超时后终止进程。
- stdout/stderr 做截断和敏感字段脱敏。
- 日志只记录模板 ID、退出码、超时状态和输出长度，不记录完整输出。

## 5. 新增测试覆盖

新增测试：

- `CommandPolicyTest`
- `CommandRunnerTest`

更新测试：

- `AgentToolCatalogTest`
- `AgentToolRegistryTest`
- `AgentPlanParserTest`
- `AgentPlanExecutorTest`

覆盖点：

- 命令模板 ID 唯一。
- 白名单命令通过策略。
- 未知命令失败。
- 危险工作目录失败。
- `QProcess` 成功执行、失败退出和超时终止。
- 命令输出脱敏和截断。
- `command.*` 进入工具目录和 Function Calling schema。
- 命令工具拒绝模型提供的额外参数。
- `AgentPlanExecutor` 可执行内部项目文件列表命令。

## 6. 验证结果

构建命令：

```powershell
cmake -S . -B build-qt -G "MinGW Makefiles" -DCMAKE_C_COMPILER=D:/QT/Tools/mingw1310_64/bin/gcc.exe -DCMAKE_CXX_COMPILER=D:/QT/Tools/mingw1310_64/bin/g++.exe
cmake --build build-qt
```

测试命令：

```powershell
ctest --test-dir build-qt --output-on-failure
```

结果：

```text
100% tests passed, 0 tests failed out of 35
```

## 7. 当前限制

- 项目目录第一版来自应用启动时的当前工作目录，后续建议做成明确配置或项目级指令。
- 外部命令模板已注册，但真实 AI 生成命令计划仍需要手工或 DeepSeek 实测验证。
- 命令执行仍是同步等待，长命令会占用当前执行流程，后续可改为异步任务。
- `git add`、`git commit`、`git push` 未开放。
- 原生 Function Calling 网络请求已在 V9.2 第一版接入，当前仍保留 JSON plan fallback。

## 8. 后续建议

V9.1 和 V9.2 已完成第一版，分别见 [V9.1 验收记录](34-v9-1-command-skills-acceptance-notes.md) 和 [V9.2 验收记录](35-v9-2-function-calling-acceptance-notes.md)。下一阶段建议进入 V10：

- 增加项目级指令文件。
- 将静态技能目录升级为外部技能文件。
- 增加受控工作记忆。
- 命令结果作为不可信数据进入后续规划。
