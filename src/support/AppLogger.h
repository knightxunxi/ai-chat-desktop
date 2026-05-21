#pragma once

#include <QString>

namespace AppLogger {

bool initialize(QString *error = nullptr);
QString logFilePath();
void setLogFilePathForTests(const QString &path);

void info(const QString &category, const QString &message);
void warning(const QString &category, const QString &message);
void error(const QString &category, const QString &message);

} // namespace AppLogger
