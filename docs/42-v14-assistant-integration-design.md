# V14 个人 AI 管家能力整合 — 设计

## 工具

| 工具 ID | 能力 | 风险 |
|---------|------|------|
| `assistant.work_journal` | 从 git log + 文件变更生成工作日报 | Low |
| `assistant.project_check` | 运行构建+测试+diff 综合检查 | Medium |
| `assistant.file_organize` | 按规则分类移动工作目录内文件 | Medium |
| `assistant.reminder` | 保存定时提醒文本到文件 | Low |

## 技术路线

利用已有 V8-V13 工具组合：git log + workspace 文件读写 + 命令执行。
