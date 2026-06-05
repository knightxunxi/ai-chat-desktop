#include "tools/registry/AgentToolCatalog.h"
#include "tools/registry/AgentToolRegistry.h"

QString agentToolRiskToString(AgentToolRisk risk)
{
    switch (risk) {
    case AgentToolRisk::Low:
        return QStringLiteral("low");
    case AgentToolRisk::Medium:
        return QStringLiteral("medium");
    case AgentToolRisk::High:
        return QStringLiteral("high");
    }

    return QStringLiteral("high");
}

bool agentToolRiskFromString(const QString &value, AgentToolRisk *risk)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("low")) {
        if (risk != nullptr) {
            *risk = AgentToolRisk::Low;
        }
        return true;
    }

    if (normalized == QStringLiteral("medium")) {
        if (risk != nullptr) {
            *risk = AgentToolRisk::Medium;
        }
        return true;
    }

    if (normalized == QStringLiteral("high")) {
        if (risk != nullptr) {
            *risk = AgentToolRisk::High;
        }
        return true;
    }

    return false;
}

AgentToolRisk maxAgentToolRisk(AgentToolRisk left, AgentToolRisk right)
{
    return static_cast<int>(left) >= static_cast<int>(right) ? left : right;
}

QString agentToolDisplayName(const AgentToolDescriptor &descriptor, AppLanguage language)
{
    return language == AppLanguage::Chinese ? descriptor.chineseName : descriptor.englishName;
}

const AgentToolDescriptor *findAgentToolDescriptor(const QVector<AgentToolDescriptor> &catalog, const QString &toolId)
{
    const QString normalizedToolId = toolId.trimmed();
    for (const AgentToolDescriptor &descriptor : catalog) {
        if (descriptor.id == normalizedToolId) {
            return &descriptor;
        }
    }

    return nullptr;
}
