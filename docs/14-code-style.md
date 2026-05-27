# 代码格式规范

本文档对应 V4-TASK-002，用于说明 AI Chat Desktop 的 C++ 代码格式规则。

## 1. 目标

代码格式规范的目标是减少无意义的风格差异，让后续功能开发、代码评审和合并更稳定。

当前阶段只新增 `.clang-format` 和说明文档，不批量格式化历史文件。原因是历史代码已经可读，强行一次性格式化会产生大量无关 diff，降低代码审查效率。

## 2. 工具

推荐使用 `clang-format`。

可以先检查本机是否可用：

```powershell
clang-format --version
```

如果未安装，可以通过 LLVM、Visual Studio 组件或其他包管理方式安装。当前项目不强制绑定具体安装路径。

## 3. 格式化范围

日常开发建议只格式化本次新增或修改的 C++ 文件，例如：

```powershell
clang-format -i src\ui\MessageWidget.cpp src\ui\MessageWidget.h
```

不建议在功能分支中执行全仓库格式化，除非该分支的唯一目标就是格式化。

## 4. 规则要点

当前 `.clang-format` 主要约束：

- 4 空格缩进。
- 不使用 Tab。
- 指针和引用保持项目既有风格，例如 `QWidget *parent`、`const QString &text`。
- 函数、类、结构体使用独立花括号。
- 控制语句花括号保持同行。
- 不自动排序 include，避免破坏 Qt 或本地头文件的既有分组。
- 行宽目标为 140，避免 Qt 字符串和信号槽连接被过度换行。

## 5. CI 状态

当前 V4-TASK-002 只提供格式规则和本地格式化方式，CI 暂不强制运行 `clang-format`。

后续如果需要强制格式检查，可以新增独立任务：

```powershell
clang-format --dry-run --Werror <files>
```

强制前应先统一决定是否对历史代码做一次专门的格式化 PR。

## 6. 验收标准

- 仓库根目录存在 `.clang-format`。
- README 或文档中说明格式化命令。
- 不产生大规模无关格式化 diff。
