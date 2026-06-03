#pragma once

#include <QMetaType>
#include <QString>

enum class RequestErrorCategory {
    Network,
    Authentication,
    Quota,
    Model,
    Server,
    Unknown,
};

Q_DECLARE_METATYPE(RequestErrorCategory)

namespace RequestErrorCategoryHelpers {

inline QString toString(RequestErrorCategory category)
{
    switch (category) {
    case RequestErrorCategory::Network:
        return QStringLiteral("network");
    case RequestErrorCategory::Authentication:
        return QStringLiteral("authentication");
    case RequestErrorCategory::Quota:
        return QStringLiteral("quota");
    case RequestErrorCategory::Model:
        return QStringLiteral("model");
    case RequestErrorCategory::Server:
        return QStringLiteral("server");
    case RequestErrorCategory::Unknown:
        return QStringLiteral("unknown");
    }

    return QStringLiteral("unknown");
}

} // namespace RequestErrorCategoryHelpers
