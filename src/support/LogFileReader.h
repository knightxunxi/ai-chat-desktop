#pragma once

#include <QString>

namespace LogFileReader {

QString readLastLines(const QString &filePath, int maxLines, QString *error = nullptr);

} // namespace LogFileReader
