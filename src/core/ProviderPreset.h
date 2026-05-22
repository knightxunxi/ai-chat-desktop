#pragma once

#include <optional>
#include <QString>
#include <QVector>

struct ProviderPreset {
    QString id;
    QString name;
    QString baseUrl;
    QString modelName;
};

namespace ProviderPresets {

QString customId();
QString customName();
QVector<ProviderPreset> defaults();
std::optional<ProviderPreset> findById(const QString &id);
std::optional<ProviderPreset> findByName(const QString &name);

} // namespace ProviderPresets
