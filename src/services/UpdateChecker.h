#pragma once

// 功能：自更新检查器占位 — 设置页保留入口，真实 GitHub Release 检查后续接入。
// N3: 当前仅提示暂未启用，不做启动时自动查询。
// 安全边界：不自动下载、不自动覆盖 exe。

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

struct UpdateInfo {
    bool hasUpdate = false;
    QString latestVersion;
    QString currentVersion;
    QString downloadUrl;
    QString releaseNotes;
    QString publishedAt;
    QString errorMessage;
    QString placeholderMessage;
};

class UpdateChecker : public QObject {
    Q_OBJECT
public:
    explicit UpdateChecker(QObject *parent = nullptr);

    // 功能：异步检查更新；使用模块：SettingsDialog 按钮。
    void checkForUpdates();

signals:
    // 功能：检查完成信号；使用模块：SettingsDialog 更新 UI。
    void updateCheckFinished(const UpdateInfo &info);

private:
    void onReplyFinished(QNetworkReply *reply);

    QNetworkAccessManager *m_nam = nullptr;
    // 当前版本号（从 CMake VERSION 定义同步）
    QString m_currentVersion = QStringLiteral("1.0");
};
