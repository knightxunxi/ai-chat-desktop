#include "support/LogFileReader.h"

#include <QFile>
#include <QStringList>
#include <QtGlobal>

namespace LogFileReader {

QString readLastLines(const QString &filePath, int maxLines, QString *error)
{
    if (error != nullptr) {
        error->clear();
    }

    if (maxLines <= 0) {
        return QString();
    }

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        if (error != nullptr) {
            *error = QStringLiteral("Failed to open log file: %1").arg(filePath);
        }
        return QString();
    }

    QStringList lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
    if (!lines.isEmpty() && lines.last().isEmpty()) {
        lines.removeLast();
    }

    const int startIndex = qMax(0, lines.size() - maxLines);
    return lines.mid(startIndex).join(QLatin1Char('\n'));
}

} // namespace LogFileReader
