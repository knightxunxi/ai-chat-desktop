# Windows 打包说明

本文档对应 V1 阶段 8 的 TASK-019，用于生成一个可在 Windows 上运行的发布目录。

## 1. 前置条件

- 已安装 Qt 6。
- Qt 的 `bin` 目录在 `PATH` 中，或者能找到 `windeployqt.exe`。
- 项目已能通过 CMake 构建。

当前 `build-qt` 使用 `MinGW Makefiles` 生成器。如果要做 Release 包，建议单独配置一个 Release 构建目录。

注意：Qt 与 MinGW 版本必须匹配。本机 Qt 6.10.2 使用 Qt 自带的 MinGW 13.1.0。若 CMake 选到其他 MinGW，可能在链接阶段失败。

## 2. Release 构建

在项目根目录执行：

```powershell
cmake -S . -B build-release-qt -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=D:/QT/Tools/mingw1310_64/bin/g++.exe
cmake --build build-release-qt
```

构建成功后，主程序通常位于：

```text
build-release-qt/AIChatDesktop.exe
```

## 3. 收集 Qt 依赖

创建发布目录：

```powershell
New-Item -ItemType Directory -Force release\AIChatDesktop
Copy-Item build-release-qt\AIChatDesktop.exe release\AIChatDesktop\
```

运行 `windeployqt`：

```powershell
windeployqt release\AIChatDesktop\AIChatDesktop.exe
```

如果 `windeployqt` 不在 `PATH` 中，使用 Qt 安装目录中的完整路径，例如：

```powershell
& "C:\Qt\6.x.x\mingw_64\bin\windeployqt.exe" release\AIChatDesktop\AIChatDesktop.exe
```

本轮执行结果：

- `build-release-qt` 构建通过。
- `ctest --test-dir build-release-qt --output-on-failure` 通过，5 个测试全部通过。
- `windeployqt release\AIChatDesktop\AIChatDesktop.exe` 执行成功。
- `windeployqt` 提示未找到 `dxcompiler.dll` 和 `dxil.dll`，并跳过 `qopensslbackend.dll`。当前应用使用 Windows TLS 后端 `qschannelbackend.dll`，但发布前仍建议在目标机器上验证 HTTPS 请求。

## 4. 发布前检查

在一台尽量干净的 Windows 环境中检查：

- 双击 `release\AIChatDesktop\AIChatDesktop.exe` 可以启动。
- 设置窗口可以保存配置。
- 语言切换后界面文案刷新。
- 发送消息可以收到流式回复。
- 关闭重启后配置和最近会话恢复。
- 缺少 API Key 或错误 API Key 时提示清楚。

## 5. 发布说明草稿

版本：0.1.0

V1 能力：

- OpenAI 兼容聊天接口。
- DeepSeek 默认配置。
- API Key、Base URL、模型名称配置。
- 中文/英文界面语言。
- 当前会话多轮上下文。
- 当前会话角色提示词。
- 流式回复展示。
- 本地配置和聊天记录保存。

已知限制：

- 支持基础多会话列表和切换，暂不支持重命名、删除、搜索等完整会话管理。
- 暂不支持停止生成。
- 暂不支持 Markdown 渲染。
- API Key 仍使用普通本地配置保存。
