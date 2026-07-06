#include "plugins/PluginManager.h"

#include "support/AppLogger.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QPluginLoader>

#include <algorithm>

PluginManager::PluginManager(QObject *parent)
    : QObject(parent)
{
}

PluginManager::~PluginManager()
{
    for (auto &entry : m_plugins) {
        if (entry.loaded) {
            unloadPlugin(entry);
        }
    }
}

void PluginManager::scanAndLoadPlugins(const QString &pluginsDir)
{
    QDir dir(pluginsDir);
    if (!dir.exists()) {
        AppLogger::info(QStringLiteral("PluginManager"),
                        QStringLiteral("Plugin directory not found: %1").arg(pluginsDir));
        return;
    }

    // 扫描所有子目录（每个插件一个子目录）
    const QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &entryInfo : entries) {
        const QString pluginDir = entryInfo.absoluteFilePath();
        const QString manifestPath = QDir(pluginDir).filePath(QStringLiteral("plugin.json"));

        if (!QFileInfo::exists(manifestPath)) {
            continue; // 子目录不含 plugin.json，跳过
        }

        PluginEntry entry;
        entry.manifestPath = manifestPath;

        if (!loadManifest(manifestPath, entry)) {
            entry.loadError = QStringLiteral("Failed to parse manifest");
            m_plugins.append(entry);
            emit pluginLoadFailed(entry.pluginId, entry.loadError);
            continue;
        }

        const auto samePlugin = [&entry](const PluginEntry &existing) {
            return existing.pluginId == entry.pluginId;
        };
        if (std::any_of(m_plugins.cbegin(), m_plugins.cend(), samePlugin)) {
            AppLogger::info(QStringLiteral("PluginManager"),
                            QStringLiteral("Skip duplicated plugin id: %1").arg(entry.pluginId));
            continue;
        }

        if (!loadLibrary(entry)) {
            entry.loadError = QStringLiteral("Failed to load DLL");
            m_plugins.append(entry);
            emit pluginLoadFailed(entry.pluginId, entry.loadError);
            continue;
        }

        entry.loaded = true;
        m_plugins.append(entry);
        AppLogger::info(QStringLiteral("PluginManager"),
                        QStringLiteral("Loaded plugin: %1 v%2").arg(entry.name, entry.version));
        emit pluginLoaded(entry.pluginId, entry.name);
    }
}

QVector<PluginEntry> PluginManager::allPlugins() const
{
    return m_plugins;
}

QVector<PluginToolInfo> PluginManager::allPluginTools() const
{
    QVector<PluginToolInfo> tools;
    for (const auto &entry : m_plugins) {
        if (entry.enabled && entry.loaded) {
            tools += entry.tools;
        }
    }
    return tools;
}

ToolResult PluginManager::executePluginTool(const QString &toolId,
                                              const QJsonObject &parameters,
                                              const AgentToolExecutionContext &context)
{
    for (const auto &entry : m_plugins) {
        if (!entry.enabled || !entry.loaded || !entry.instance) continue;

        for (const auto &tool : entry.tools) {
            if (tool.id == toolId) {
                return entry.instance->execute(toolId, parameters, context);
            }
        }
    }
    return ToolResult::failure(QStringLiteral("Plugin tool not found: %1").arg(toolId));
}

void PluginManager::setPluginEnabled(const QString &pluginId, bool enabled)
{
    for (auto &entry : m_plugins) {
        if (entry.pluginId == pluginId) {
            entry.enabled = enabled;
            if (!enabled && entry.loaded) {
                unloadPlugin(entry);
            } else if (enabled && !entry.loaded) {
                if (loadLibrary(entry)) {
                    entry.loaded = true;
                    entry.loadError.clear();
                    emit pluginLoaded(entry.pluginId, entry.name);
                } else {
                    emit pluginLoadFailed(entry.pluginId, entry.loadError);
                }
            }
            return;
        }
    }
}

bool PluginManager::loadManifest(const QString &manifestPath, PluginEntry &entry)
{
    QFile file(manifestPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return false;
    }

    QJsonObject root = doc.object();
    entry.pluginId = root.value(QStringLiteral("id")).toString();
    entry.name = root.value(QStringLiteral("name")).toString();
    entry.version = root.value(QStringLiteral("version")).toString();

    // 解析 tools
    const QJsonArray toolsArray = root.value(QStringLiteral("tools")).toArray();
    for (const QJsonValue &val : toolsArray) {
        QJsonObject obj = val.toObject();
        PluginToolInfo tool;
        tool.id = obj.value(QStringLiteral("id")).toString();
        tool.name = obj.value(QStringLiteral("name")).toString();
        tool.description = obj.value(QStringLiteral("description")).toString();
        tool.risk = static_cast<AgentToolRisk>(obj.value(QStringLiteral("risk")).toInt(0));
        tool.schema = obj.value(QStringLiteral("schema")).toObject();
        if (!tool.id.isEmpty()) {
            entry.tools.append(tool);
        }
    }

    // 记录 DLL 路径（manifest 同目录下的 .dll）
    QDir manifestDir = QFileInfo(manifestPath).absoluteDir();
    entry.libraryPath = manifestDir.filePath(entry.pluginId + QStringLiteral(".dll"));
    // fallback: 尝试任意 .dll
    if (!QFileInfo::exists(entry.libraryPath)) {
        const QFileInfoList dlls = manifestDir.entryInfoList({QStringLiteral("*.dll")}, QDir::Files);
        if (!dlls.isEmpty()) {
            entry.libraryPath = dlls.first().absoluteFilePath();
        }
    }

    return !entry.pluginId.isEmpty();
}

bool PluginManager::loadLibrary(PluginEntry &entry)
{
    if (entry.libraryPath.isEmpty() || !QFileInfo::exists(entry.libraryPath)) {
        return false;
    }

    auto *loader = new QPluginLoader(entry.libraryPath, this);
    QObject *instance = loader->instance();

    if (!instance) {
        entry.loadError = loader->errorString();
        delete loader;
        return false;
    }

    PluginInterface *plugin = qobject_cast<PluginInterface *>(instance);
    if (!plugin) {
        entry.loadError = QStringLiteral("Plugin does not implement PluginInterface");
        delete loader;
        return false;
    }

    entry.instance = plugin;
    return true;
}

void PluginManager::unloadPlugin(PluginEntry &entry)
{
    if (entry.instance) {
        // QPluginLoader owns the instance; deleting the loader unloads the library
        // Find the loader associated with this entry
        const auto children = findChildren<QPluginLoader *>();
        for (auto *loader : children) {
            if (loader->instance() == entry.instance) {
                loader->unload();
                delete loader;
                break;
            }
        }
        entry.instance = nullptr;
    }
    entry.loaded = false;
    emit pluginUnloaded(entry.pluginId);
}
