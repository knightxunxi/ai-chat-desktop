# Windows 打包说明

本文档对应 V1 阶段 8 的 TASK-019，用于生成一个可在 Windows 上运行的发布目录。

## 1. 前置条件

- 已安装 Qt 6。
- Qt 的 `bin` 目录在 `PATH` 中，或者能找到 `windeployqt.exe`。
- 项目已能通过 CMake 构建。
- 本文命令以 PowerShell 为例。

当前 `build-qt` 使用 `MinGW Makefiles` 生成器。如果要做 Release 包，建议单独配置一个 Release 构建目录。

注意：Qt 与 MinGW 版本必须匹配。本机 Qt 6.10.2 使用 Qt 自带的 MinGW 13.1.0。若 CMake 选到其他 MinGW，可能在链接阶段失败。

推荐先确认工具路径：

```powershell
Get-Command qt-cmake, cmake, windeployqt -ErrorAction SilentlyContinue
Get-Command D:\QT\Tools\mingw1310_64\bin\g++.exe -ErrorAction SilentlyContinue
```

如果 `qt-cmake` 或 `windeployqt` 不在 `PATH` 中，可以使用完整路径：

```powershell
& "D:\QT\6.10.2\mingw_64\bin\qt-cmake.bat" --version
& "D:\QT\6.10.2\mingw_64\bin\windeployqt.exe" --version
```

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

建议同时运行 Release 测试：

```powershell
ctest --test-dir build-release-qt --output-on-failure
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
& "D:\QT\6.10.2\mingw_64\bin\windeployqt.exe" release\AIChatDesktop\AIChatDesktop.exe
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
- 生成中点击停止可以取消当前请求。
- 消息复制、Markdown 代码块展示和角色提示词模板可用。
- 日志文件能记录请求开始和完成。
- 关闭重启后配置和最近会话恢复。
- 缺少 API Key 或错误 API Key 时提示清楚。

如果要上传 GitHub Release，建议只上传发布目录压缩包，不提交 `release/` 目录到 Git。当前 `.gitignore` 已忽略 `release/`。

推荐压缩命令：

```powershell
Compress-Archive -Path release\AIChatDesktop\* -DestinationPath release\AIChatDesktop-0.2.0-windows.zip -Force
```

压缩包生成后，建议解压到一个临时目录再运行一次，确认没有依赖遗漏。

## 5. 常见问题

### 5.1 链接阶段出现 Qt EntryPoint 或 MinGW 相关错误

通常是 CMake 选到了错误的 MinGW。请使用 Qt 套件匹配的编译器：

```powershell
-DCMAKE_CXX_COMPILER=D:/QT/Tools/mingw1310_64/bin/g++.exe
```

必要时删除旧构建目录后重新配置：

```powershell
Remove-Item -Recurse -Force build-release-qt
cmake -S . -B build-release-qt -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=D:/QT/Tools/mingw1310_64/bin/g++.exe
```

### 5.2 双击 exe 提示缺少 Qt DLL

说明尚未运行 `windeployqt`，或运行对象不是发布目录中的 exe。请对 `release\AIChatDesktop\AIChatDesktop.exe` 执行 `windeployqt`。

### 5.3 HTTPS 请求失败

发布目录需要包含 Qt 网络和 TLS 相关插件。`windeployqt` 通常会复制所需插件。若目标机器请求失败，请检查发布目录中的 `tls/` 插件，以及目标机器的系统证书和网络代理。

### 5.4 API Key 没有随发布包一起带走

这是预期行为。API Key 保存在用户本机配置中，不随发布包分发。每台机器首次使用时都需要在设置窗口中填写自己的 API Key。

## 6. 发布说明草稿

版本：0.2.0

V2 能力：

- OpenAI 兼容聊天接口。
- DeepSeek 默认配置。
- API Key、Base URL、模型名称配置。
- 中文/英文界面语言。
- 当前会话多轮上下文。
- 当前会话角色提示词。
- 流式回复展示。
- 本地配置和聊天记录保存。
- 多会话列表、新建、切换和删除。
- 停止生成。
- 消息复制。
- AI 消息基础 Markdown 渲染和代码块展示。
- 基础日志记录。
- 角色提示词模板。

已知限制：

- API Key 仍使用普通本地配置保存。
- 多会话暂不支持重命名和搜索。
- Markdown 暂不支持复杂代码高亮。
- 日志暂不支持应用内查看。
- 角色提示词模板暂不支持导入导出和云同步。
