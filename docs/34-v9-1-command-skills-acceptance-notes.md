# V9.1 验收记录：开发者命令技能与项目目录配置化

## 1. 版本目标

V9.1 的目标是把 V9 的白名单命令能力从“单个命令工具”推进到“可复用开发者流程”，并把命令项目目录从启动目录改为用户可配置项。

本阶段仍然不包含：

- 自动 `git add`、`git commit`、`git push`。
- 任意 PowerShell/CMD 字符串。
- 原生 Function Calling 网络请求。
- 设备输入模拟。

## 2. 完成内容

已完成：

- `AppConfig` 新增 `agentProjectDirectory`。
- `ConfigStorage` 支持保存和加载 `agent/projectDirectory`。
- `SettingsDialog` 新增 Agent 项目目录输入框。
- `MainWindow` 打开 Agent 计划窗口时使用配置中的项目目录。
- 新增 `AgentCommandSkillCatalog`。
- `AgentPlanPromptBuilder` 在规划 Prompt 和统一模式 Prompt 中加入推荐开发者命令技能。
- 新增 `AgentCommandSkillCatalogTest`。

## 3. 内置开发者命令技能

第一版内置技能：

| 技能 ID | 名称 | 步骤 |
| --- | --- | --- |
| `developer.inspect_changes` | 检查当前改动 | `command.git_status` -> `command.git_diff_stat` |
| `developer.pre_commit_check` | 提交前检查 | `command.git_diff_check` -> `command.cmake_build` -> `command.ctest` |
| `developer.build_and_test` | 构建并测试 | `command.cmake_build` -> `command.ctest` |
| `developer.diagnose_tests` | 定位测试失败 | `command.ctest` -> `command.git_status` |

技能本身不绕过工具安全策略。技能只会展开为 `command.*` 工具步骤，执行时仍经过 `AgentToolRegistry`、`CommandPolicy` 和 `CommandRunner`。

## 4. 安全边界

- 技能不能新增工具权限。
- 技能不能修改命令参数。
- 技能步骤仍使用空参数对象。
- 命令项目目录由用户配置，不再依赖应用启动目录。
- 项目目录仍由 `CommandPolicy` 校验，危险目录会被拒绝。

## 5. 新增测试覆盖

新增测试：

- `AgentCommandSkillCatalogTest`

更新测试：

- `ConfigStorageTest`
- `SettingsDialogSmokeTest`
- `AgentPlanPromptBuilderTest`

覆盖点：

- 项目目录可保存和加载。
- 设置窗口能读回 Agent 项目目录。
- 技能 ID 唯一。
- 技能步骤全部使用 `command.*` 工具。
- 提交前检查技能可展开为 diff check、build、ctest 三步。
- Agent Prompt 包含推荐开发者命令技能。

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
100% tests passed, 0 tests failed out of 36
```

## 7. 当前限制

- 技能目录仍是 C++ 静态定义，尚未支持读取外部技能文件。
- 技能尚未在 UI 中单独展示或一键触发。
- 真实 AI 是否稳定按技能展开计划还需要手工验证。
- 原生 Function Calling 已在 V9.2 第一版接入，见 [V9.2 验收记录](35-v9-2-function-calling-acceptance-notes.md)。

## 8. 后续建议

V9.2 已完成第一版，下一阶段建议进入 V10：

- 增加项目级指令文件。
- 将静态技能目录升级为外部技能文件。
- 增加受控工作记忆。
- 继续补真实 AI 手工验证。

也可以在 V10 前补一个小任务：增加技能列表 UI 或一键生成技能计划入口。
