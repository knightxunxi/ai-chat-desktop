# V12 电脑感知设计

## 1. 目标

让应用"看见"桌面环境：枚举窗口、检测前台窗口、截图保存、OCR 识别，并记录操作链路。

## 2. 工具清单

| 工具 ID | 能力 | 风险 | 技术路线 |
|---------|------|------|----------|
| `system.list_windows` | 枚举所有可见窗口 | Low | Win32 `EnumWindows` |
| `system.foreground_window` | 获取前台窗口标题 | Low | Win32 `GetForegroundWindow` |
| `system.capture_screen` | 截取屏幕保存到工作目录 | Medium | Qt `QScreen::grabWindow` |
| `system.ocr_text` | 从截图文件提取文字 | Medium | Windows OCR API 或占位 |

## 3. 参数 Schema

### system.list_windows
```json
{"type": "object", "properties": {"max_count": {"type": "integer", "description": "最大窗口数，默认50"}}}
```

### system.capture_screen
```json
{"type": "object", "properties": {"output_path": {"type": "string", "description": "工作目录内保存路径，如 screenshot.png"}}, "required": ["output_path"]}
```

### system.ocr_text
```json
{"type": "object", "properties": {"image_path": {"type": "string", "description": "工作目录内图片相对路径"}}, "required": ["image_path"]}
```

## 4. 新增文件

```
src/tools/WindowDetector.h/.cpp       — 窗口枚举和前台检测
src/tools/ScreenCaptureService.h/.cpp — 屏幕截图
src/tools/OcrService.h/.cpp           — OCR 文字提取
tests/tools/WindowDetectorTest.cpp
tests/tools/ScreenCaptureServiceTest.cpp
```

## 5. 安全约束

- 截图仅保存到工作目录
- OCR 结果标记为不可信数据
- 不自动截图敏感窗口
- 不记录屏幕敏感内容到日志
