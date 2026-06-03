# V9 命令执行安全设计

本文档对应 V9-TASK-001，用于定义 AI Chat Desktop 的受控命令执行边界。V9 的目标不是开放任意终端，而是在本地策略校验后执行少量固定命令模板。

## 1. 核心原则

- 命令必须来自本地白名单，AI 不能直接提供任意 PowerShell/CMD 字符串。
- 命令执行使用“程序 + 参数数组”，不通过 shell 拼接。
- 命令工作目录必须是项目目录或后续明确允许的安全目录。
- 命令必须有超时，超时后终止进程。
- 命令输出只返回摘要和截断文本，日志不记录完整输出。
- 命令输出视为不可信数据，不能改变工具权限、工作目录或安全策略。

## 2. 第一版允许命令

| 工具 ID | 程序 | 参数 | 风险 | 超时 |
| --- | --- | --- | --- | --- |
| `command.git_status` | `git` | `status --short --branch` | low | 15 秒 |
| `command.git_diff_check` | `git` | `diff --check` | low | 20 秒 |
| `command.git_diff_stat` | `git` | `diff --stat` | low | 20 秒 |
| `command.cmake_build` | `cmake` | `--build build-qt` | medium | 60 秒 |
| `command.ctest` | `ctest` | `--test-dir build-qt --output-on-failure` | medium | 60 秒 |
| `command.list_project_files` | 内部 Qt 文件枚举 | 列出项目根目录 | low | 不适用 |

说明：

- `git add`、`git commit`、`git push` 暂不开放。
- `cmake_build` 和 `ctest` 只使用固定参数，不允许 AI 修改构建目录或追加参数。
- `command.list_project_files` 不启动外部进程，只用于帮助 Agent 观察项目根目录。

## 3. 禁止命令

V9 明确禁止：

- `powershell`
- `pwsh`
- `cmd`
- `del`
- `erase`
- `rd`
- `rmdir`
- `Remove-Item`
- `reg`
- `sc`
- `net`
- `shutdown`
- `format`
- `diskpart`
- 任意包含 `&&`、`||`、`;`、`|`、重定向符号的 shell 片段

这些命令即使由 AI 生成，也不能通过本地策略校验。

## 4. 工作目录策略

命令默认工作目录是项目根目录。当前开发阶段由应用启动时传入，例如开发环境下通常是：

```text
D:\C1\CodeXX
```

工作目录必须满足：

- 非空。
- 存在。
- 是目录。
- 不是磁盘根目录。
- 不是 Windows、Program Files、ProgramData 等系统关键目录。
- 不是用户主目录根部。

第一版不允许 AI 自行指定 cwd。

## 5. 输出策略

命令执行结果返回给 UI 的内容包含：

- 命令模板 ID。
- 实际程序和参数。
- 工作目录。
- 退出码。
- 是否超时。
- stdout 摘要。
- stderr 摘要。
- stdout/stderr 原始长度。

输出限制：

- stdout 和 stderr 分别截断。
- 疑似 API Key、Bearer Token、password、secret、token 字段需要脱敏。
- 日志只记录输出长度和退出码，不记录完整输出。

## 6. Agent 策略

Agent 只能建议 `command.*` 工具 ID。计划解析器仍需要校验：

- `toolId` 必须存在于工具注册表。
- 风险等级以本地工具目录为准。
- `parameters` 必须是对象。
- 命令工具第一版不接受任意用户参数。

执行后：

- 成功结果可以展示给用户。
- 失败结果需要暂停连续执行。
- 命令输出回传给 AI 前必须视为不可信数据。

## 7. 测试要求

自动化测试至少覆盖：

- 白名单命令可以通过 `CommandPolicy`。
- 未知命令失败。
- 禁止命令失败。
- 空 cwd、根目录、系统目录、用户主目录失败。
- `CommandRunner` 可运行程序和参数数组。
- 非零退出码返回失败。
- 超时命令会被终止。
- 输出过长会截断。
- 输出中的敏感字段会脱敏。
- 命令工具能进入 `AgentToolRegistry`。
- `AgentPlanExecutor` 能执行允许的命令工具。

## 8. 后续扩展

V9.1 可以把命令组合成技能，例如：

- 提交前检查。
- 构建并测试。
- 定位测试失败。

V9.2 再考虑原生 Function Calling 接入，但工具调用结果仍必须经过本文档定义的本地策略。
