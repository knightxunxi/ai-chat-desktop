#include "core/ProviderPreset.h"

namespace {

constexpr auto DeepSeekId = "deepseek";
constexpr auto OpenAIId = "openai";
constexpr auto CustomId = "custom";
constexpr auto CustomName = "Custom";

bool equalsIgnoreCase(const QString &left, const QString &right)
{
    return left.compare(right, Qt::CaseInsensitive) == 0;
}

} // namespace

namespace ProviderPresets {

QString customId()
{
    return QString::fromLatin1(CustomId);
}

QString customName()
{
    return QString::fromLatin1(CustomName);
}

QVector<ProviderPreset> defaults()
{
    return {
        ProviderPreset{
            QString::fromLatin1(DeepSeekId),
            QStringLiteral("DeepSeek"),
            QStringLiteral("https://api.deepseek.com"),
            QStringLiteral("deepseek-v4-flash")
        },
        ProviderPreset{
            QString::fromLatin1(OpenAIId),
            QStringLiteral("OpenAI"),
            QStringLiteral("https://api.openai.com/v1"),
            QStringLiteral("gpt-4.1-mini")
        }
    };
}

std::optional<ProviderPreset> findById(const QString &id)
{
    for (const ProviderPreset &preset : defaults()) {
        if (equalsIgnoreCase(preset.id, id.trimmed())) {
            return preset;
        }
    }

    return std::nullopt;
}

std::optional<ProviderPreset> findByName(const QString &name)
{
    for (const ProviderPreset &preset : defaults()) {
        if (equalsIgnoreCase(preset.name, name.trimmed())) {
            return preset;
        }
    }

    return std::nullopt;
}

} // namespace ProviderPresets
