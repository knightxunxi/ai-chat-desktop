#pragma once

#include "core/AppConfig.h"

class ConfigStorage
{
public:
    AppConfig load() const;
    void save(const AppConfig &config) const;
};
