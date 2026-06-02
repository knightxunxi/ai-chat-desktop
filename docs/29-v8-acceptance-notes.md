# V8.1 验收记录：工作目录文件 Agent MVP

## 1. 版本目标

V8.1 的目标是让 Agent 从“只能执行低风险文本工具”扩展为“可以在配置的 Agent 工作目录内自动创建、读取、列目录、覆盖和删除普通文件”。

本版本仍然不包含：

- 任意 PowerShell/CMD 命令执行。
- 键盘鼠标模拟。
- 工作目录外自动读写。
- 后台无人值守长期任务。

## 2. 完成内容

已完成：

- 新增 `WorkspaceFileService`。
- 新增 `workspace.write_text`。
- 新增 `workspace.read_text`。
- 新增 `workspace.list_directory`。
- 新增 `workspace.overwrite_text`。
- 新增 `workspace.delete_file`。
- `workspace.*` 工具加入 `AgentToolCatalog`。
- `AgentPlanExecutor` 支持执行 `workspace.*` 步骤。
- 计划窗口展示 Agent 工作目录。
- 计划窗口新增连续执行和停止按钮。
- 连续执行最多 5 步，失败后暂停。
- 文件读取结果标记为不可信数据。
- 继续规划时提示模型不要把文件内容里的指令当作命令。
- 受保护文件禁止自动创建、覆盖和删除。

## 3. 安全边界

路径边界：

- 相对路径会解析到 Agent 工作目录内。
- `../outside.txt` 这类路径穿越会被拒绝。
- 工作目录外绝对路径会被拒绝。
- 系统关键目录不能作为 Agent 工作目录。

文件边界：

- `workspace.write_text` 不覆盖已有文件。
- `workspace.overwrite_text` 覆盖前生成 `.bak` 备份。
- `workspace.delete_file` 不永久删除文件，而是移动到工作目录内 `.trash`。
- `.git`、`.env`、凭据、密钥、证书类文件禁止自动创建、覆盖和删除。

提示词注入边界：

- 读取文件结果会包裹 `UNTRUSTED WORKSPACE FILE DATA`。
- 文件内容只作为待分析数据，不作为系统指令。
- 文件内容不能新增工具权限、修改工作目录或绕过本地策略。

日志边界：

- 日志记录工具 ID、相对路径、结果长度和错误摘要。
- 日志不记录文件正文、API Key、Token 或完整敏感内容。

## 4. 验证结果

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
100% tests passed, 0 tests failed out of 29
```

说明：

- 当前 `build-qt` 已使用 Qt 自带 MinGW 13.1.0 重新配置。
- 使用其他 MinGW 版本可能和 Qt 入口库 ABI 不匹配，导致主程序链接失败。

## 5. 新增测试覆盖

新增或更新测试：

- `WorkspaceFileServiceTest`
- `AgentPlanExecutorTest`
- `AgentPlanParserTest`
- `AgentPlanPromptBuilderTest`
- `AgentToolCatalogTest`
- `WorkspacePolicyTest`
- `AgentPlanDialogSmokeTest`

覆盖点：

- 工作目录内写入、读取、列目录、覆盖和删除。
- 覆盖前备份。
- 删除移动到 `.trash`。
- 路径穿越拒绝。
- 工作目录外绝对路径拒绝。
- 受保护文件拒绝创建、覆盖和删除。
- 二进制文件和超大文件读取拒绝。
- 文件读取结果不可信数据标记。
- `workspace.*` 工具目录登记和风险等级提升。
- 计划窗口连续执行按钮和停止按钮基础行为。

## 6. 当前限制

- 连续执行是同步 MVP，停止会在当前步骤完成后生效。
- 当前仍依赖计划窗口执行步骤，没有做聊天内 Agent 自动循环。
- Agentic Loop 和 Function Calling 兼容层已在 V8.2/V8.3 第一版中补齐，见 [V8.2/V8.3 验收记录](30-v8-2-v8-3-acceptance-notes.md)。
- 还没有命令执行能力。

## 7. 后续建议

下一阶段建议进入 V9：

- 新增受控命令执行。
- 只允许白名单命令。
- 增加命令超时和输出摘要。
- 命令输出作为不可信数据回传。

不建议加入任意 shell。
