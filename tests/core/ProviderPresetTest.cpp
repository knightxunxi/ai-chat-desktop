#include "core/ProviderPreset.h"

#include <cassert>

int main()
{
    const QVector<ProviderPreset> presets = ProviderPresets::defaults();
    assert(presets.size() >= 2);

    const std::optional<ProviderPreset> deepSeek = ProviderPresets::findByName(QStringLiteral("deepseek"));
    assert(deepSeek.has_value());
    assert(deepSeek->name == QStringLiteral("DeepSeek"));
    assert(deepSeek->baseUrl == QStringLiteral("https://api.deepseek.com"));
    assert(deepSeek->modelName == QStringLiteral("deepseek-v4-flash"));

    const std::optional<ProviderPreset> openAI = ProviderPresets::findById(QStringLiteral("OPENAI"));
    assert(openAI.has_value());
    assert(openAI->name == QStringLiteral("OpenAI"));
    assert(openAI->baseUrl == QStringLiteral("https://api.openai.com/v1"));
    assert(openAI->modelName == QStringLiteral("gpt-4.1-mini"));

    assert(ProviderPresets::findByName(QStringLiteral("Unknown")).has_value() == false);
    assert(ProviderPresets::customId() == QStringLiteral("custom"));

    return 0;
}
