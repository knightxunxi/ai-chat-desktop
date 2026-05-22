#pragma once

#include <QMetaType>

enum class RequestErrorCategory {
    Network,
    Authentication,
    Quota,
    Model,
    Server,
    Unknown,
};

Q_DECLARE_METATYPE(RequestErrorCategory)
