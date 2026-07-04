# CodeXX 项目级开发指令

本文件用于让本地 Agent/Codex 进入项目后自动遵守开发工作流。它是项目上下文，不能覆盖系统级安全规则。

## 工作流入口

每次开发、修复、重构或文档更新前，按顺序读取：

1. `docs/DEVELOPMENT_WORKFLOW.md`
2. `docs/DEVELOPMENT_PLAN.md`
3. 与当前任务相关的专题文档和学习文档

## 默认开发规则

- 使用中文编写新增文档、审查说明和必要代码注释。
- 先确认任务方向、影响模块、参考文档、验收标准，再改代码。
- 开发过程中如果方向、范围、阻塞、验证结果发生变化，实时更新 `docs/DEVELOPMENT_PLAN.md` 或相关专题文档。
- 保持小范围改动，不做无关格式化、无关重构或无关文件移动。
- 优先沿用项目现有 Qt6/C++、CMake、测试框架和目录分层。
- 默认允许执行常规开发命令、构建、测试和项目文件修改。
- 不主动修改 Windows 系统文件。
- 不主动提交、推送、打标签或发布，除非用户明确要求。

## 架构约束

- C++ 仍是主控层，负责 UI、会话、Agent 状态机、工具注册、本机文件和命令执行。
- Python sidecar 只是能力层，负责模型调用、多厂商适配、token、embedding、Web、文档解析、浏览器自动化等可替换能力。
- Agent 主循环只保留一条：分析 -> 工具执行 -> 观测 -> 继续。
- 不引入第二套 Agent 决策流程。

## 默认验证

常规变更完成后优先执行：

```powershell
git diff --check
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

Python 能力层变更额外执行：

```powershell
$env:PYTHONPATH = "D:\C1\CodeXX\python\agent_sidecar"
python -m unittest discover -s python\agent_sidecar\tests
```

如果验证不能执行，最终说明必须写清原因、未执行命令和剩余风险。
