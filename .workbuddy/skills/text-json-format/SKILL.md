---
name: text.json_format
description: 将用户提供的 JSON 文本转为带缩进的格式化输出，方便阅读和编辑。
triggers:
  - JSON格式化
  - json format
  - 格式化JSON
  - format json
  - 美化JSON
  - prettify json
version: 1.0.0
priority: 20
enabled: true
author: CodeXX Builtin
---

## JSON 格式化工具 (text.json_format)

**功能**: 将紧凑或混乱的 JSON 文本转为缩进清晰的格式化输出。

**适用场景**:
- 用户给了你一段难以阅读的紧凑 JSON，需要你帮格式化
- API 返回的 JSON 响应需要美化后展示
- 调试时需要看清 JSON 结构

**调用方式**: 使用 Function Calling 调用 `json_format`，传入 `{"input": "<JSON文本>"}`。

**注意事项**:
- 输入必须是合法的 JSON 文本，否则工具会返回错误
- 工具仅做格式化，不修改 JSON 结构和数据
- 输出结果中不包含敏感信息处理——如有敏感字段请先告知用户
