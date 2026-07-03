# CodeXX V18 系列开发总结

> 日期：2026-06-05 | 测试：65/65 全部通过，零回归

---

## 目标

将 CodeXX 从一个"纯聊天+简单文件操作"的应用，升级为**完整桌面 Agent 平台**，对标 Claude Code / Codex CLI 的能力集。

---

## V18 核心优化（7 项）

Agent 循环质量提升，防止死循环、上下文膨胀、幻觉终止。

| 项 | 说明 |
|----|------|
| **工具结果裁剪** | observation 注入前截断超长输出（>50 行 → 首20+尾20） |
| **重复动作检测** | 同步+异步两路径指纹检测，3 次重复→终止 |
| **Stop Hook** | 提示词注入：done=true 前必须验证目标完成 |
| **Microcompact** | 早期 observation 压缩为摘要，保留最近 5 条完整 |
| **轻量子代理** | `agent.explore` — 只读工具 + 30s 超时 + 同步等待 |
| **Reactive 压缩** | context_length_exceeded → 压缩 observation → 最多重试 3 次 |
| **输出截断续接** | finish_reason=length → 自动发送"从断点继续" |

---

## 新增工具全集（37 个）

### 文件操作（10）
| 工具 | 功能 |
|------|------|
| `file.edit_text` | old_str→new_str 精确替换（Claude Code 风格） |
| `file.grep` | 正则递归搜索，返回 file:line:content |
| `file.delete_directory` | 递归删除目录 + 系统目录保护 |
| `file.copy` | 复制文件/目录（递归） |
| `file.move` | 移动/重命名（跨盘符 copy+remove 回退） |
| `file.append_text` | 追加文本到文件末尾 |
| `file.get_info` | 获取文件元信息（大小/时间/权限） |
| `file.archive` | 压缩为 zip（PowerShell Compress-Archive） |
| `file.extract` | 解压 zip（PowerShell Expand-Archive） |
| `file.watch` | QFileSystemWatcher 文件监听 |

### 命令执行（1）
| 工具 | 功能 |
|------|------|
| `command.bash` | 通用 Shell 命令（替代 6 个硬编码）+ 危险命令黑名单 |

### 桌面操作（10）
| 工具 | 功能 |
|------|------|
| `system.clipboard_read` | 读系统剪贴板 |
| `system.clipboard_write` | 写系统剪贴板 |
| `input.mouse_click` | 鼠标单击（左/中/右键） |
| `input.mouse_scroll` | 滚轮滚动 |
| `input.mouse_drag` | 鼠标拖拽 (x1,y1)→(x2,y2) |
| `input.mouse_position` | 获取当前鼠标坐标 |
| `input.key_press` | 单键按下（enter/tab/esc/方向键等） |

### 桌面感知（5）
| 工具 | 功能 |
|------|------|
| `system.active_control` | UIA/Win32 焦点控件信息 |
| `system.screen_size` | 主屏幕分辨率 |
| `system.get_window_rect` | 按标题查找窗口位置大小 |
| `system.get_selected_text` | Ctrl+C 获取选中文本 |
| `system.wait_for_window` | 轮询等待窗口出现 |

### 网络请求（3）
| 工具 | 功能 |
|------|------|
| `web.http_get` | HTTP GET 请求（15s 超时，64KiB 截断） |
| `web.http_post` | HTTP POST（JSON body） |
| `web.download_file` | 流式下载文件（32MiB 上限，60s 超时） |

### 代码运行（1）
| 工具 | 功能 |
|------|------|
| `code.run` | 运行 Python/JS 代码（临时文件 + 30s 超时 + 8KiB 裁剪） |

### 系统集成（1）
| 工具 | 功能 |
|------|------|
| `system.open_url` | 默认浏览器打开 URL |

### BugFix（1）
| 项 | 说明 |
|----|------|
| **workspace.delete_file** | 改用 `QFile::moveToTrash()` → Windows Recycle Bin |

---

## UI 优化（1 项）

| 项 | 说明 |
|----|------|
| **AgentStepGroupWidget** | 多步骤折叠为摘要卡片 `▶ Completed: N steps — tool1, tool2...`，点击展开 |

---

## 智能工具选择（V18.6）

### 意图分类
7 种意图自动识别：代码编辑 / 代码搜索 / 构建测试 / 文件管理 / 桌面操作 / 网络请求 / Shell 命令

### 工具排序
- 匹配工具排前面加 ⭐ 标记
- 不匹配工具软限制 30 个
- 非匹配工具超过 30 时截断并提示

### 最佳实践注入
8 条静态工具使用技巧注入提示词（如"先读再改"、"截图→OCR→坐标→操作"）

### 工具序列记忆
完成后写入 `.workbuddy/memory/tool-usage.md`，下次相似任务可参考

---

## 自动修复闭环（V18.5）

Agent 编辑文件后自动触发：

```
编辑操作 → cmake --build (60s) → ctest (60s) → 结果注入 observation → AI 自修复
```

---

## 改进的 Bug（今日累计 4 个）

| 问题 | 修复 |
|------|------|
| Agent 计划成功后仍跑满 15 轮 | 成功直接结束，失败才进 OODA |
| delete_file 假删除 | `moveToTrash()` → Recycle Bin |
| LLM 仍认为工具仅限工作目录 | 更新 5 个工具描述 + 2 个提示词 |
| 统一模式计划不完整 | 提示词强调"包含所有步骤" |

---

## 改动量统计

| 文件 | 类型 | 改动 |
|------|------|------|
| `AgentToolRegistry.cpp` | 工具注册 | +600 行（37 个新工具） |
| `FileInteractionService.h/.cpp` | 服务层 | +7 函数 ~240 行 |
| `InputSimulator.h/.cpp` | 服务层 | +6 函数 ~170 行 |
| `AgentLoopPromptBuilder.h/.cpp` | 提示词 | +200 行（智能排序+记忆+指导） |
| `AgentOrchestrator.h/.cpp` | 编排 | +80 行（autoFix + 记录） |
| `ApplicationController.h/.cpp` | 控制 | +60 行（压缩/续接） |
| `StreamParser.h/.cpp` | 流解析 | +5 行（截断检测） |
| `WorkspaceFileService.cpp` | 文件服务 | +5 行（Recycle Bin） |
| `AgentStepGroupWidget.h/.cpp` | UI 新组件 | ~100 行 |
| `MainWindow.h/.cpp` | UI | +15 行（步骤分组） |
| `CMakeLists.txt` (ui) | 构建 | +2 行 |

**总计**: 约 1500 行新增/修改代码，65/65 测试零回归。

---

## 能力全景图

```
文件   read  save  write  edit  copy  move  append  delete  grep  info  archive  extract  watch  .......... 14
命令   bash  git_status  cmake_build  ctest  find_files  list                                                           6
桌面   clipboard_r/w  mouse_click  mouse_scroll  mouse_drag  mouse_pos  key_press                                       6
感知   capture_screen  ocr  list_windows  foreground  active_control  screen_size  rect  wait_window  selected_text    9
网络   http_get  http_post  download_file  open_url                                                                      4
代码   code_run  explore                                                                                                 2
系统   clipboard  env  path  open_url                                                                                    4
其他   json  markdown  work_journal  reminder  project_check  file_organize                                             6
────────────────────────────────────────────────────────────────────────────────────────────────────────
总计   51 个工具，覆盖文件/命令/桌面/感知/网络/代码/系统 7 大领域
```
