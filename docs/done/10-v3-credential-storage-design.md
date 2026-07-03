# V3 API Key 安全存储方案

本文档对应 V3-TASK-002，用于确定 AI Chat Desktop 在 Windows 上保存 API Key 的方案。当前版本只考虑 Windows 桌面使用场景，暂不设计 macOS/Linux 迁移。

## 1. 背景

当前应用通过 `ConfigStorage` 使用 `QSettings` 保存配置，其中包含 `api/apiKey`。这种方式实现简单，但 API Key 会作为普通本地配置存在，不适合作为 V3 后续功能继续依赖的基础。

V3 的目标是：

- API Key 不再写入普通 `QSettings`。
- Base URL、模型名称、语言等非敏感配置仍保留在 `QSettings`。
- 兼容用户已有配置，尽量避免升级后需要重新填写。
- 日志、导出文件和错误提示不暴露 API Key。

## 2. 方案选择

选型：Windows Credential Manager。

原因：

- 项目当前只面向 Windows 使用，不需要为了跨平台引入额外依赖。
- Windows Credential Manager 是系统级凭据存储，适合保存 API Key 这类用户级机密。
- Win32 Credential Management API 可以直接从 C++ 调用，构建上只需要包含 `wincred.h` 并链接 `Advapi32`。

暂不采用：

- Qt Keychain：适合跨平台场景，但当前 V3 明确不考虑平台迁移，引入依赖的收益不高。
- 继续使用 `QSettings`：只能作为非敏感配置存储，不再保存 API Key。

## 3. API 设计

新增 `CredentialStorage` 边界，用来隔离凭据读写逻辑。它的目的不是承诺跨平台，而是让业务代码不直接依赖 Win32 API，并方便自动化测试使用 fake 实现。

建议接口能力：

```cpp
class CredentialStorage
{
public:
    virtual ~CredentialStorage() = default;

    virtual QString readApiKey(QString *error = nullptr) const = 0;
    virtual bool writeApiKey(const QString &apiKey, QString *error = nullptr) = 0;
    virtual bool deleteApiKey(QString *error = nullptr) = 0;
};
```

Windows 实现建议命名为 `WindowsCredentialStorage`。

## 4. Windows Credential Manager 映射

建议使用：

- 类型：`CRED_TYPE_GENERIC`
- TargetName：`AIChatDesktop/OpenAICompatibleApiKey`
- UserName：`AIChatDesktop`
- CredentialBlob：UTF-8 编码后的 API Key
- Persist：`CRED_PERSIST_LOCAL_MACHINE` 或 `CRED_PERSIST_ENTERPRISE`

当前更推荐 `CRED_PERSIST_LOCAL_MACHINE`，因为这个项目优先满足本机桌面应用场景，避免把行为扩大到企业域漫游凭据。

需要用到的 Win32 API：

- `CredWriteW`：新增或覆盖凭据。
- `CredReadW`：读取当前用户会话下的凭据。
- `CredDeleteW`：清除保存的 API Key。
- `CredFree`：释放 `CredReadW` 返回的凭据内存。

CMake 需要在 Windows 构建中链接：

```cmake
target_link_libraries(AIChatDesktop PRIVATE Advapi32)
```

测试目标如果直接链接 Windows 实现，也需要同样链接 `Advapi32`。

## 5. 配置拆分

`QSettings` 继续保存：

- 服务商名称或自定义服务商标记。
- Base URL。
- 模型名称。
- 界面语言。
- 其他 UI 偏好。

`QSettings` 不再保存：

- API Key。
- 任何可恢复完整鉴权信息的敏感字段。

`AppConfig` 可以暂时保留 `apiKey` 字段，作为运行时配置对象使用。真正的持久化读写由 `CredentialStorage` 负责。

## 6. 迁移策略

升级后的读取流程：

1. 从 `QSettings` 读取非敏感配置。
2. 从 Windows Credential Manager 读取 API Key。
3. 如果 Credential Manager 没有 API Key，但旧 `QSettings` 存在 `api/apiKey`：
   - 尝试写入 Windows Credential Manager。
   - 写入成功后删除 `QSettings` 中的 `api/apiKey`。
   - 写入失败时不记录 key 内容，只提示用户重新保存设置。

保存设置时：

1. 非敏感字段写入 `QSettings`。
2. API Key 写入 Windows Credential Manager。
3. 如果 API Key 为空，删除 Windows Credential Manager 中的旧凭据。
4. 无论写入成功或失败，都不能把 API Key 打到日志。

## 7. 错误处理

建议把 Win32 错误转换为应用内部的简短错误类型：

- 未找到凭据：不是致命错误，视为没有保存 API Key。
- 写入失败：设置窗口提示“API Key 保存失败，请重新检查系统凭据权限或稍后重试”。
- 删除失败：提示“API Key 清除失败”，但不阻止非敏感配置保存。

日志要求：

- 可以记录操作类型、是否成功和错误码。
- 不记录 API Key。
- 不记录包含 API Key 的完整配置对象。

## 8. 测试策略

自动化测试优先覆盖业务行为：

- `ConfigStorage` 不再把新 API Key 写入 `QSettings`。
- 旧 `QSettings` API Key 能触发迁移逻辑。
- 凭据写入失败时，不丢失非敏感配置。
- 空 API Key 会触发删除凭据。

为了避免测试污染真实 Windows Credential Manager，单元测试使用内存 fake：

```cpp
class FakeCredentialStorage final : public CredentialStorage
{
    // 在测试进程内保存 API Key，并可模拟读写失败。
};
```

真实 Windows Credential Manager 行为通过手工验收确认。

## 9. 手工验收

实现完成后建议按以下步骤检查：

1. 从旧版本配置升级，确认原 API Key 仍能用于发送请求。
2. 重新打开设置窗口并保存，确认聊天请求可用。
3. 检查普通配置中不再出现 `api/apiKey`。
4. 清空 API Key 后保存，确认后续请求提示缺少 API Key。
5. 查看日志，确认没有 API Key 或聊天正文。

## 10. 参考资料

- Microsoft Learn: [Credentials Management](https://learn.microsoft.com/en-us/windows/win32/secauthn/credentials-management)
- Microsoft Learn: [CredWriteW function](https://learn.microsoft.com/en-us/windows/win32/api/wincred/nf-wincred-credwritew)
- Microsoft Learn: [CredReadW function](https://learn.microsoft.com/en-us/windows/win32/api/wincred/nf-wincred-credreadw)
- Microsoft Learn: [CredDeleteW function](https://learn.microsoft.com/en-us/windows/win32/api/wincred/nf-wincred-creddeletew)
- Microsoft Learn: [CredFree function](https://learn.microsoft.com/en-us/windows/win32/api/wincred/nf-wincred-credfree)
