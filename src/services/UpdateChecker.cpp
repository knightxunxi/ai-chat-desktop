#include "services/UpdateChecker.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &UpdateChecker::onReplyFinished);
}

void UpdateChecker::checkForUpdates()
{
    UpdateInfo info;
    info.currentVersion = m_currentVersion;
    info.latestVersion = m_currentVersion;
    info.hasUpdate = false;
    info.placeholderMessage = QStringLiteral("自动更新检查暂未启用；当前仅保留 v1.0 占位入口，后续接入真实 GitHub Release 仓库。");
    emit updateCheckFinished(info);
}

void UpdateChecker::onReplyFinished(QNetworkReply *reply)
{
    UpdateInfo info;
    info.currentVersion = m_currentVersion;

    if (reply->error() != QNetworkReply::NoError) {
        info.errorMessage = QStringLiteral("Network error: %1").arg(reply->errorString());
        emit updateCheckFinished(info);
        reply->deleteLater();
        return;
    }

    const QByteArray data = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        info.errorMessage = QStringLiteral("JSON parse error: %1").arg(parseError.errorString());
        emit updateCheckFinished(info);
        reply->deleteLater();
        return;
    }

    const QJsonObject root = doc.object();

    // 检查 API 错误（例如 rate limit）
    if (root.contains(QStringLiteral("message"))) {
        info.errorMessage = root.value(QStringLiteral("message")).toString();
        emit updateCheckFinished(info);
        reply->deleteLater();
        return;
    }

    // 提取版本号（去掉 "v" 前缀，如 "v1.1" → "1.1"）
    QString tagName = root.value(QStringLiteral("tag_name")).toString();
    if (tagName.startsWith(QLatin1Char('v'))) {
        tagName = tagName.mid(1);
    }

    // 提取下载 URL（assets 中第一个 zip）
    const QJsonArray assets = root.value(QStringLiteral("assets")).toArray();
    for (const QJsonValue &val : assets) {
        const QJsonObject asset = val.toObject();
        const QString name = asset.value(QStringLiteral("name")).toString();
        if (name.endsWith(QStringLiteral(".zip"))) {
            info.downloadUrl = asset.value(QStringLiteral("browser_download_url")).toString();
            break;
        }
    }

    // 提取发布说明
    info.releaseNotes = root.value(QStringLiteral("body")).toString().left(2000);
    info.publishedAt = root.value(QStringLiteral("published_at")).toString();
    info.latestVersion = tagName;

    // 版本比较（简单字符串比较，v1.9 < v1.10 需注意）
    const QStringList currentParts = m_currentVersion.split(QLatin1Char('.'));
    const QStringList latestParts = tagName.split(QLatin1Char('.'));
    // 按数字逐段比较
    for (int i = 0; i < qMax(currentParts.size(), latestParts.size()); ++i) {
        const int cur = i < currentParts.size() ? currentParts[i].toInt() : 0;
        const int lat = i < latestParts.size() ? latestParts[i].toInt() : 0;
        if (lat > cur) {
            info.hasUpdate = true;
            break;
        } else if (lat < cur) {
            break; // 当前版本比最新还新（开发版）
        }
    }

    emit updateCheckFinished(info);
    reply->deleteLater();
}
