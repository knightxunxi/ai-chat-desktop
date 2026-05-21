#include "support/AppLogger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>

namespace {

QMutex &loggerMutex()
{
    static QMutex mutex;
    return mutex;
}

QString &configuredLogFilePath()
{
    static QString path;
    return path;
}

QString normalizedMessage(QString message)
{
    message.replace(QLatin1Char('\r'), QLatin1Char(' '));
    message.replace(QLatin1Char('\n'), QLatin1Char(' '));
    message.replace(QRegularExpression(QStringLiteral("Bearer\\s+[^\\s,;]+"), QRegularExpression::CaseInsensitiveOption),
                    QStringLiteral("Bearer [REDACTED]"));
    message.replace(QRegularExpression(QStringLiteral("((?:api[_-]?key|apikey)\\s*[:=]\\s*)[^\\s,;]+"), QRegularExpression::CaseInsensitiveOption),
                    QStringLiteral("\\1[REDACTED]"));
    return message.trimmed();
}

QString defaultLogFilePath()
{
    QString directoryPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (directoryPath.trimmed().isEmpty()) {
        directoryPath = QDir::temp().filePath(QStringLiteral("AIChatDesktop"));
    }

    return QDir(directoryPath).filePath(QStringLiteral("ai-chat-desktop.log"));
}

bool ensureLogFilePath(QString *error)
{
    QString &path = configuredLogFilePath();
    if (path.trimmed().isEmpty()) {
        path = defaultLogFilePath();
    }

    QDir directory(QFileInfo(path).absolutePath());
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        if (error != nullptr) {
            *error = QStringLiteral("Failed to create log directory: %1").arg(directory.absolutePath());
        }
        return false;
    }

    QFile file(path);
    if (!file.open(QFile::Append | QFile::Text)) {
        if (error != nullptr) {
            *error = QStringLiteral("Failed to open log file: %1").arg(path);
        }
        return false;
    }

    return true;
}

void writeLine(const QString &level, const QString &category, const QString &message)
{
    QMutexLocker locker(&loggerMutex());

    if (!ensureLogFilePath(nullptr)) {
        return;
    }

    QFile file(configuredLogFilePath());
    if (!file.open(QFile::Append | QFile::Text)) {
        return;
    }

    QTextStream stream(&file);
    stream << QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
           << " [" << level << "]"
           << " [" << normalizedMessage(category) << "] "
           << normalizedMessage(message)
           << '\n';
}

} // namespace

namespace AppLogger {

bool initialize(QString *error)
{
    QMutexLocker locker(&loggerMutex());
    return ensureLogFilePath(error);
}

QString logFilePath()
{
    QMutexLocker locker(&loggerMutex());
    if (configuredLogFilePath().trimmed().isEmpty()) {
        configuredLogFilePath() = defaultLogFilePath();
    }

    return configuredLogFilePath();
}

void setLogFilePathForTests(const QString &path)
{
    QMutexLocker locker(&loggerMutex());
    configuredLogFilePath() = path;
}

void info(const QString &category, const QString &message)
{
    writeLine(QStringLiteral("INFO"), category, message);
}

void warning(const QString &category, const QString &message)
{
    writeLine(QStringLiteral("WARN"), category, message);
}

void error(const QString &category, const QString &message)
{
    writeLine(QStringLiteral("ERROR"), category, message);
}

} // namespace AppLogger
