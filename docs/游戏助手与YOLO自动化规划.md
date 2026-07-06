# 游戏助手与 YOLO 自动化规划

> 日期：2026-07-06
> 状态：⬚ 计划中
> 范围：CodeXX 主页面新增“游戏助手”入口；规划“游戏自动化助手”和“逆向学习助手”两个方向。
> 重要边界：本文不规划反作弊绕过、注入执行、内存篡改、封包修改或在线游戏作弊能力。

---

## 1. 任务卡

- 任务编号：#35 游戏自动化
- 任务名称：游戏助手与 YOLO 自动化方向规划
- 目标：
  - 在 CodeXX 主页面上方规划新增“游戏助手”按钮。
  - 在游戏助手中规划两个模块：游戏自动化助手、逆向学习助手。
  - 将 YOLOv5 图像识别、动态识别选项、数据集、训练、观察模式和安全自动化串成可落地路线。
  - 结合 CodeXX 现有 UI、Agent、工具系统、Python sidecar、插件和调度能力设计接入边界。
- 非目标：
  - 不生成源码实现。
  - 不规划商业/在线游戏作弊、绕过反作弊、DLL 注入、进程隐藏、内存写入、封包修改。
  - 不把 Python sidecar 变成第二套 Agent 主循环。
- 影响模块：
  - `src/ui/MainWindow.cpp`：顶部入口按钮规划。
  - `src/ui/`：未来新增 `GameAssistantDialog`。
  - `src/app/ApplicationController*`：未来承接业务协调。
  - `src/tools/`：未来复用感知、输入、窗口校验工具。
  - `src/services/PythonSidecar*`、`python/agent_sidecar/`：未来承接 YOLO 训练和推理能力。
  - `src/plugins/`：未来可把视觉模型、标注器、逆向学习能力做成受控插件。
- MCP 查询结论：
  - `D-C1-CodeXX` 索引状态为 `ready`，当前图谱可用。
  - 主窗口顶部按钮在 `MainWindow::setupUi` 中创建，现有入口包括角色提示词、工具、日志、设置、调度任务和主题切换。
  - 命令面板已在 `MainWindow::setupUi` 中维护命令列表，后续可同步加入“游戏助手”。
  - 现有本机能力已经包含屏幕截图、窗口检测、OCR、前台窗口校验、鼠标键盘模拟。
  - Python sidecar 已承接模型、浏览器、Web、文档等可替换能力，适合扩展 YOLO 训练和推理。
  - 插件系统已有 `PluginManager`、`plugin.json`、工具注册接入，适合承载实验性或可选能力。
- 验收标准：
  - 本规划能明确 UI 入口、模块拆分、数据流、安全边界、实施阶段和验收标准。
  - 逆向学习助手必须保留为授权学习/沙盒分析方向，不落入外挂或注入执行工具。

---

## 2. 总体定位

“游戏助手”建议定位为 CodeXX 的一个受控工作台，而不是聊天窗口中的普通工具按钮集合。

它应服务两类合法场景：

1. 游戏自动化助手
   - 面向单机、自研游戏、测试环境或获得授权的自动化验证场景。
   - 核心链路是“项目管理 -> 识别选项 -> 图片/标注 -> YOLO 训练 -> 观察模式 -> 安全动作策略”。

2. 逆向学习助手
   - 面向自有程序、教学样例、CTF/安全实验室、合法调试笔记管理。
   - 核心链路是“资料整理 -> 静态观察 -> 笔记与地址表记录 -> 风险提示 -> 学习报告”。
   - 不提供对商业游戏进程的注入、写内存、封包篡改或反作弊绕过能力。

命名建议：

- 顶部按钮：`游戏助手`
- 弹窗标题：`游戏助手`
- 内部页签：
  - `自动化助手`
  - `数据集与训练`
  - `观察与回放`
  - `策略与安全`
  - `逆向学习`

其中“逆向学习”比“Cheat Engine 注入助手”更适合作为产品入口名称，因为它能明确合法学习边界，避免 CodeXX 被设计成外挂平台。

---

## 3. CodeXX 现有基础

### 3.1 UI 入口基础

当前主页面由 `MainWindow::setupUi` 构建：

```text
chatHeader
  titleGroup
  Role Prompt
  Tools
  Agent Plan(hidden)
  Logs
  Settings
  Scheduled Tasks
  Theme Toggle
```

建议在 `Tools` 之后、`Logs` 之前加入：

```text
Game Assistant / 游戏助手
```

原因：

- “游戏助手”是一个专题工作台，和 `Tools` 同级，比普通工具窗口更重。
- 它需要承载数据集、训练任务、模型版本、回放、运行日志等多视图，不适合塞入 `ToolsDialog`。
- 放在顶部按钮中能让用户明确进入一个独立模式，而不是误以为它只是一次性工具调用。

命令面板也应同步加入：

```text
game_assistant -> 游戏助手
```

斜杠命令可后置规划：

```text
/game
/game-assistant
```

### 3.2 Agent 与工具基础

CodeXX 已有以下能力可复用：

| 方向 | 现有基础 | 规划用途 |
| --- | --- | --- |
| Agent 主循环 | `ApplicationController` + `AgentOrchestrator` | 让 Agent 解释训练结果、整理数据问题、生成策略建议 |
| 工具注册 | `AgentToolRegistry` | 后续注册受控的游戏助手工具 |
| 屏幕感知 | `ScreenCaptureService`、`WindowDetector`、`OcrService` | 游戏窗口截图、窗口标题校验、OCR 辅助识别 |
| 输入控制 | `InputSimulator`、`ForegroundValidator` | 自动化动作执行前校验前台窗口 |
| Python 能力层 | `PythonSidecarClient`、`python/agent_sidecar` | YOLO 训练、推理、数据集检查、可选 ONNX 推理 |
| 插件系统 | `PluginManager`、`plugin.json` | 视觉模型插件、标注工具插件、逆向学习插件 |
| 调度任务 | `TaskScheduler` | 定时数据清理、训练完成提醒、回放测试 |
| 日志 | `LogViewerDialog`、AppLogger | 训练日志、检测日志、动作日志、安全审计 |

### 3.3 需要新增的核心对象

未来实现时建议新增：

```text
src/ui/GameAssistantDialog.*
src/app/GameAssistantController.*
src/core/GameAssistantModels.*
src/storage/GameAssistantStorage.*
python/agent_sidecar/agent_sidecar/vision_*.py
```

如果先做最小闭环，可以把训练和推理先放在 Python sidecar，C++ 只做 UI、任务调度、文件路径管理和结果展示。

---

## 4. 信息架构

### 4.1 顶部入口

新增按钮：

```text
游戏助手
```

行为：

- 点击后打开 `GameAssistantDialog`。
- 默认进入 `自动化助手` 页。
- 如果没有项目，显示项目创建向导。
- 如果存在项目，显示最近一次打开的游戏项目。

按钮状态：

- Python sidecar 不可用时，按钮仍可打开，但训练/推理页显示能力缺失提示。
- 自动化运行中，按钮可显示运行状态标识，但不建议在主聊天区持续刷状态。
- 逆向学习页默认受限，需要用户确认合法用途说明后才能启用。

### 4.2 GameAssistantDialog 页签

建议页签如下：

| 页签 | 作用 | MVP 优先级 |
| --- | --- | :---: |
| 自动化助手 | 游戏项目、识别目标、运行模式总览 | P0 |
| 数据集与训练 | 图片导入、标注状态、YOLO 数据集、训练任务 | P0 |
| 观察与回放 | 截图检测、视频/截图回放、检测框预览 | P1 |
| 策略与安全 | 动作策略、阈值、冷却、停止热键、前台窗口校验 | P1 |
| 逆向学习 | CE 笔记、静态观察、地址表资料管理、合法性提示 | P2 |

### 4.3 工作模式

必须提供三档模式：

1. 观察模式
   - 只截图、检测、画框、记录。
   - 不点击、不按键。
   - 默认模式。

2. 半自动模式
   - Agent 给出动作建议。
   - 用户确认后执行。
   - 用于验证策略。

3. 自动模式
   - 只允许在授权单机/测试项目中启用。
   - 必须启用停止热键、前台窗口校验、动作频率限制。
   - 关键动作需要连续帧确认。

---

## 5. 游戏自动化助手规划

### 5.1 项目创建

用户先创建一个游戏项目：

- 项目名
- 游戏窗口标题或进程显示名
- 截图区域
- 分辨率
- 默认模型
- 数据目录
- 是否允许自动化动作
- 合法用途声明

推荐数据目录：

```text
%APPDATA%/CodeXX/game-assistant/
  games/
    {game_id}/
      project.json
      options.json
      raw/
      dataset/
      runs/
      replay/
      logs/
```

如果用户希望项目随工作区迁移，也可以支持 workspace 内目录：

```text
{workspace}/.codexx/game-assistant/
```

### 5.2 动态识别选项

每个游戏项目可以动态添加识别选项：

- 选项 key：例如 `enemy`、`start_button`、`coin`
- 显示名：例如“敌人”“开始按钮”“金币”
- 类型：目标、按钮、状态条、危险区域、文本区域
- YOLO 类别编号：创建后锁定
- 默认置信度阈值
- 动作提示：仅观察、点击中心、避开区域、OCR 辅助、状态判断

重要规则：

- YOLO 类别编号只追加，不重排。
- 删除选项只能软删除，不能复用旧编号。
- 选项文件夹只是原始素材入口，不等于 YOLO 数据集。
- 目标检测必须有图片和对应边界框标注。

### 5.3 图片和标注流程

推荐流程：

```text
导入图片/截图/视频抽帧
  -> 放入 raw/{option_key}
  -> 人工或半自动标注 bounding box
  -> 生成 dataset/images + dataset/labels
  -> 生成 data.yaml
  -> 数据质量检查
  -> 启动训练
```

MVP 可先支持：

- 导入图片目录。
- 为每个选项显示样本数量。
- 导入已有 YOLO 标签。
- 生成 `data.yaml`。
- 执行基础检查：
  - 图片无标签。
  - 标签无图片。
  - 类别编号越界。
  - 空标注文件。
  - 归一化坐标不在 `0..1`。

标注工具可以分阶段：

1. 第一版：只导入已有 YOLO 标签。
2. 第二版：提供简单框选 UI。
3. 第三版：接入外部标注工具或插件。
4. 第四版：用旧模型做预标注，再人工审核。

### 5.4 YOLO 训练

训练职责建议放到 Python sidecar：

```text
C++ UI
  -> PythonSidecarClient
    -> vision.dataset.validate
    -> vision.yolo.train
    -> vision.yolo.detect
```

建议新增 sidecar 能力：

| 能力 | 输入 | 输出 |
| --- | --- | --- |
| `vision.dataset.validate` | 项目目录、data.yaml | 检查报告 |
| `vision.yolo.train` | data.yaml、base model、epochs、imgsz、batch | run_id、日志、best.pt |
| `vision.yolo.detect` | image/video/screenshot、weights、阈值 | 检测框 JSON |
| `vision.yolo.export` | best.pt、目标格式 | onnx 或其他部署产物 |

训练策略：

- 默认从 `yolov5n` 或 `yolov5s` 微调。
- 第一版不做从零训练。
- 训练任务必须有日志和可取消状态。
- 模型版本不能覆盖，必须保留 `train_001`、`train_002` 这类版本。
- 训练结果至少展示：
  - Precision
  - Recall
  - mAP
  - 混淆情况
  - 样本数量
  - 训练耗时

### 5.5 推理与观察

观察模式数据流：

```text
ScreenCaptureService / Python 截图
  -> YOLO detect
  -> 检测框还原到窗口坐标
  -> UI 预览
  -> replay/detections 日志
```

推理结果结构：

```json
{
  "frame_id": "20260706-001",
  "window_title": "Example Game",
  "image_size": [1920, 1080],
  "detections": [
    {
      "class_key": "enemy",
      "class_index": 0,
      "confidence": 0.92,
      "box": [100, 120, 220, 260]
    }
  ]
}
```

优化方向：

- 只截取游戏窗口或 ROI。
- 对低频目标降低检测频率。
- 小目标场景提高 `imgsz`，再权衡 FPS。
- 连续帧平滑，避免单帧误检触发动作。
- 失败截图自动进入待标注区。

### 5.6 自动化策略

不要让 YOLO 直接执行动作。必须拆成：

```text
感知层 Vision
  -> 决策层 Strategy
    -> 动作层 Action
      -> 安全层 Safety
```

策略配置示例：

```json
{
  "target": "start_button",
  "mode": "click_center",
  "confidence_min": 0.88,
  "confirm_frames": 3,
  "cooldown_ms": 1500,
  "foreground_title_required": "Example Game",
  "max_actions_per_minute": 20
}
```

安全要求：

- 默认自动模式关闭。
- 必须检查前台窗口标题。
- 必须有全局停止热键。
- 必须限制动作频率。
- 必须记录每一次动作和触发原因。
- 低置信度时只提示，不执行。
- 异常弹窗或状态未知时进入暂停状态。

---

## 6. 逆向学习助手规划

### 6.1 功能定位

该模块不应设计成“注入/外挂助手”，而应设计为“逆向学习与资料整理助手”。

允许方向：

- 自有程序或教学样例的静态分析笔记。
- Cheat Engine 表结构说明和手工记录管理。
- 反汇编/调试概念解释。
- 字符串、导入表、导出表、PE 元数据的只读摘要。
- CTF 或本地沙盒练习记录。
- Agent 辅助整理观察结果、写学习报告、生成合法实验步骤清单。

禁止方向：

- 对商业游戏或在线游戏进程执行注入。
- 读写游戏内存、修改数值、Hook 游戏函数。
- 生成 DLL 注入器、驱动、反调试绕过、反作弊绕过。
- 封包抓取、篡改、重放。
- 自动寻找可作弊地址、自动生成修改脚本。
- 绕过验证码、登录、付费、风控或平台限制。

### 6.2 UI 文案建议

页签名称：

```text
逆向学习
```

首屏提示：

```text
该区域仅用于自有程序、教学样例和授权安全实验的学习记录。
CodeXX 不提供游戏注入、反作弊绕过、内存篡改或外挂生成能力。
```

按钮命名避免：

- `注入`
- `Attach 游戏`
- `修改内存`
- `绕过检测`

建议按钮：

- `导入学习笔记`
- `导入 CE 表说明`
- `分析本地样例文件`
- `生成学习报告`
- `查看安全边界`

### 6.3 Cheat Engine 资料助手

可以支持的内容：

- 导入用户手工维护的 CE 表文件路径或说明文档。
- 解析用户自己写的说明文本，整理为字段、地址含义、观察记录。
- 对“指针、多级指针、扫描、断点、模块基址”等概念做解释。
- 将一次学习过程导出为 Markdown 报告。

不支持：

- 自动连接目标进程。
- 自动扫描地址。
- 自动写入内存。
- 自动生成 Cheat Table 脚本。
- 自动生成注入器。

如果未来确实需要对自研玩具程序做调试实验，必须单独走“本地沙盒实验”能力：

- 目标程序必须在 CodeXX 创建的 sandbox 目录中。
- 目标程序必须由用户显式标记为自有样例。
- 只允许只读元数据分析。
- 不提供可复用于第三方游戏的注入代码或内存修改实现。

### 6.4 与游戏自动化助手的边界

两个模块可以共享：

- 项目管理。
- 日志系统。
- Agent 解释和报告能力。
- 文件导入。
- 安全声明和审计记录。

两个模块不能共享：

- 自动化动作策略不能调用逆向学习模块产出的地址或注入逻辑。
- YOLO 识别结果不能触发内存修改。
- 逆向学习模块不能绕过游戏窗口校验和动作安全限制。

---

## 7. Agent 接入方式

游戏助手不应新增第二套 Agent。建议复用现有 Agent 主循环，并新增受控工具或工作台动作。

### 7.1 可开放给 Agent 的能力

P0/P1 可开放：

```text
game.project.list
game.project.describe
game.dataset.validate
game.training.summarize
game.detection.summarize
game.strategy.review
```

P2 后开放：

```text
game.dataset.import_images
game.training.start
game.replay.run
```

暂不开放：

```text
game.action.click
game.action.keypress
game.automation.start
```

原因：动作执行需要用户确认和 UI 安全上下文，不适合一开始让 Agent 直接触发。

### 7.2 Agent 适合做什么

Agent 适合：

- 检查数据集质量报告并解释问题。
- 根据误检/漏检样本建议补充数据。
- 根据训练指标建议调整参数。
- 审查自动化策略是否过于激进。
- 生成测试清单和回放用例。
- 整理逆向学习笔记。

Agent 不适合：

- 独立决定启用自动模式。
- 绕过安全阈值。
- 直接操作在线游戏。
- 生成注入或内存修改方案。

---

## 8. 存储设计

MVP 可以使用 JSON 文件，稳定后迁移 SQLite。

### 8.1 JSON 文件

```text
project.json
options.json
training_runs.json
automation_profiles.json
reverse_notes.json
```

### 8.2 后续 SQLite 表

```text
game_projects
recognition_options
image_assets
annotations
dataset_versions
training_runs
detection_runs
automation_profiles
action_logs
reverse_learning_notes
safety_acknowledgements
```

### 8.3 审计日志

必须记录：

- 用户何时启用自动模式。
- 使用了哪个模型版本。
- 当前前台窗口标题。
- 每次动作的检测框、置信度、策略 ID。
- 停止原因。
- 逆向学习页的合法用途确认时间。

---

## 9. 实施路线

### Phase 0：文档和安全边界

- 完成本规划。
- 明确逆向学习助手边界。
- 明确不做注入、内存修改和反作弊绕过。

### Phase 1：UI 壳和项目管理

- 主页面顶部新增 `游戏助手` 按钮。
- 新增 `GameAssistantDialog` 空壳。
- 支持创建游戏项目。
- 支持添加动态识别选项。
- 支持查看项目目录和样本数量。

验收：

- 主窗口启动正常。
- 游戏助手可打开和关闭。
- 创建项目后能持久化。
- 识别选项类别编号稳定。

### Phase 2：数据集生成

- 支持图片导入。
- 支持导入 YOLO 标签。
- 生成 YOLO 数据集目录。
- 生成 `data.yaml`。
- 执行数据质量检查。

验收：

- 能从一个小样本项目生成可训练数据集。
- 能发现缺失标签、类别越界、图片标签不匹配。

### Phase 3：YOLO 训练接入

- Python sidecar 新增 `vision.dataset.validate`。
- Python sidecar 新增 `vision.yolo.train`。
- C++ UI 展示训练进度和训练结果。
- 模型版本化保存。

验收：

- 能启动一次训练。
- 能保存 `best.pt` 和训练参数。
- 能展示指标摘要。

### Phase 4：观察模式

- 对静态截图做检测。
- 对窗口截图做检测。
- 绘制检测框。
- 保存检测日志。

验收：

- 不执行任何输入动作。
- 检测结果能和窗口坐标对应。
- 低置信度结果可过滤。

### Phase 5：策略与半自动

- 配置动作策略。
- 支持用户确认后点击或按键。
- 增加前台窗口校验、停止热键、冷却时间。

验收：

- 前台窗口不匹配时拒绝动作。
- 用户取消时不执行动作。
- 每次动作都有日志。

### Phase 6：受控自动模式

- 仅对授权项目开放。
- 需要连续帧确认。
- 需要最大动作频率限制。
- 异常状态自动暂停。

验收：

- 自动模式默认关闭。
- 可随时停止。
- 回放日志能复盘每个动作原因。

### Phase 7：逆向学习助手

- 新增学习笔记区。
- 支持导入 CE 表说明或 Markdown 笔记。
- 支持 Agent 整理概念和学习报告。
- 支持只读样例文件元数据摘要。

验收：

- 不存在注入、写内存、Attach 第三方游戏进程的入口。
- 首次使用必须确认合法用途声明。
- 导出的报告包含安全边界说明。

---

## 10. 风险和缓解

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| 用户把功能用于在线游戏作弊 | 法律、账号、项目声誉风险 | 默认只支持授权项目；文档和 UI 明确禁止；不提供注入/内存修改 |
| 动态选项缺少边界框标注 | YOLO 训练不可用 | 数据集检查明确提示；先支持导入 YOLO 标签 |
| 类别编号重排 | 旧模型识别含义错乱 | 类别编号只追加，删除只软删除 |
| 误检触发动作 | 游戏状态错乱 | 观察模式默认；连续帧确认；置信度阈值；半自动验证 |
| 分辨率变化 | 点击坐标错误 | 运行前校验窗口标题、分辨率、截图区域 |
| sidecar 依赖重 | 安装复杂 | 训练能力做可选依赖；UI 显示缺失项 |
| GPU 不可用 | 训练慢 | 支持 CPU 低样本验证；提示用户使用外部训练 |
| 自动模式不可控 | 用户体验和安全风险 | 停止热键、频率限制、动作日志、异常暂停 |
| 逆向学习边界滑坡 | 变成外挂工具 | 模块命名、能力白名单、禁用注入/写内存能力 |

---

## 11. 推荐优先级

最推荐顺序：

1. 游戏助手 UI 壳。
2. 游戏项目和动态识别选项。
3. 图片/标签导入和数据集检查。
4. YOLO 训练任务。
5. 观察模式。
6. 回放测试。
7. 半自动策略。
8. 受控自动模式。
9. 逆向学习助手。

不建议第一版做：

- 自动注入。
- 内存读写。
- 在线游戏自动化。
- 强化学习控制。
- 多游戏通用策略引擎。
- 训练平台级标注系统。
- Agent 直接执行自动化动作。

---

## 12. 最小 MVP 范围

如果只做一个 7-10 天 MVP，建议范围压缩为：

- 顶部 `游戏助手` 按钮。
- `GameAssistantDialog` 三页：
  - 项目
  - 数据集
  - 观察
- 支持创建项目。
- 支持动态识别选项。
- 支持导入图片和已有 YOLO 标签。
- 支持生成 `data.yaml`。
- 支持调用 Python sidecar 做一次 YOLO 训练。
- 支持对单张截图检测并画框。
- 不做自动点击。
- 不做逆向学习页的执行能力，只放合法用途说明和笔记入口。

这样能先验证“CodeXX + YOLO 游戏视觉工作台”的主路径，再决定是否投入自动化动作和逆向学习模块。

---

## 13. 参考资料

- `docs/DEVELOPMENT_PLAN.md`：#35 游戏自动化。
- `docs/codebase-memory-mcp-callgraph.md`：主流程、工具注册、Python sidecar 边界。
- `docs/done/49-v19-python-agent-capability-layer-plan.md`：Python 能力层历史设计。
- `docs/插件系统相关实现`：未来可选能力扩展方向以 `PluginManager` 为入口。
- Ultralytics YOLOv5：https://github.com/ultralytics/yolov5
- YOLOv5 自定义数据训练：https://docs.ultralytics.com/yolov5/tutorials/train_custom_data/
