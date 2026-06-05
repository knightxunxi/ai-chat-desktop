#include "tools/dev/CsvDataService.h"

#include "tools/core/WorkspacePolicy.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

// 学习注释：CsvDataService 提供工作目录内的 CSV 读写能力。
// 所有路径通过 WorkspacePolicy 校验，限定在工作目录内。

namespace {

// 功能：解析单行 CSV（支持引号转义）；使用模块：readCsv。
QStringList parseCsvLine(const QString &line)
{
    QStringList fields;
    QString current;
    bool inQuotes = false;

    for (int i = 0; i < line.size(); ++i) {
        const QChar ch = line.at(i);
        if (inQuotes) {
            if (ch == QLatin1Char('"')) {
                if (i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"')) {
                    current += QLatin1Char('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                current += ch;
            }
        } else {
            if (ch == QLatin1Char('"')) {
                inQuotes = true;
            } else if (ch == QLatin1Char(',')) {
                fields.append(current.trimmed());
                current.clear();
            } else {
                current += ch;
            }
        }
    }
    fields.append(current.trimmed());
    return fields;
}

// 功能：将字符串转义为 CSV 安全格式；使用模块：writeCsv。
QString escapeCsvField(const QString &field)
{
    if (field.contains(QLatin1Char(',')) || field.contains(QLatin1Char('"')) || field.contains(QLatin1Char('\n'))) {
        QString escaped = field;
        escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
        return QLatin1Char('"') + escaped + QLatin1Char('"');
    }
    return field;
}

constexpr int MaxCsvRows = 2000;
constexpr int MaxCsvFileBytes = 5 * 1024 * 1024;

} // namespace

namespace CsvDataService {

ToolResult readCsv(const QString &workspaceDirectory,
                   const QString &path,
                   int maxRows,
                   bool hasHeader)
{
    if (path.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("CSV path must not be empty."));
    }
    if (maxRows <= 0 || maxRows > MaxCsvRows) {
        maxRows = MaxCsvRows;
    }

    const QString resolvedPath = QDir(workspaceDirectory).filePath(path);
    if (!WorkspacePolicy::isPathInsideWorkspace(workspaceDirectory, resolvedPath)) {
        return ToolResult::failure(QStringLiteral("Path is outside workspace directory."));
    }

    QFile file(resolvedPath);
    if (!file.exists()) {
        return ToolResult::failure(QStringLiteral("CSV file does not exist: %1").arg(path));
    }
    if (file.size() > MaxCsvFileBytes) {
        return ToolResult::failure(QStringLiteral("CSV file too large (>5 MB): %1").arg(path));
    }
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        return ToolResult::failure(QStringLiteral("Cannot open CSV file: %1").arg(file.errorString()));
    }

    QTextStream stream(&file);

    QStringList headerRow;
    if (hasHeader && !stream.atEnd()) {
        headerRow = parseCsvLine(stream.readLine());
    }

    QVector<QStringList> dataRows;
    while (!stream.atEnd() && dataRows.size() < maxRows) {
        const QString line = stream.readLine().trimmed();
        if (!line.isEmpty()) {
            dataRows.append(parseCsvLine(line));
        }
    }
    file.close();

    // 构建输出
    QString output;
    output += QStringLiteral("=== CSV Data (%1 rows, %2 path) ===\n")
                  .arg(dataRows.size())
                  .arg(path);

    if (!headerRow.isEmpty()) {
        output += QStringLiteral("Columns: %1\n").arg(headerRow.join(QStringLiteral(", ")));
        output += QStringLiteral("---\n");
        for (int row = 0; row < dataRows.size() && row < 20; ++row) {
            output += QStringLiteral("[%1] ").arg(row + 1);
            for (int col = 0; col < headerRow.size() && col < dataRows.at(row).size(); ++col) {
                output += QStringLiteral("%1: %2  ").arg(headerRow.at(col), dataRows.at(row).at(col));
            }
            output += QLatin1Char('\n');
        }
    } else {
        output += QStringLiteral("Columns: %1\n").arg(dataRows.isEmpty() ? 0 : dataRows.first().size());
        output += QStringLiteral("---\n");
        for (int row = 0; row < dataRows.size() && row < 20; ++row) {
            output += QStringLiteral("[%1] %2\n").arg(row + 1).arg(dataRows.at(row).join(QStringLiteral(", ")));
        }
    }

    if (dataRows.size() > 20) {
        output += QStringLiteral("... (%1 more rows)\n").arg(dataRows.size() - 20);
    }

    return ToolResult::success(output);
}

ToolResult writeCsv(const QString &workspaceDirectory,
                    const QString &path,
                    const QVector<QStringList> &rows,
                    const QStringList &header)
{
    if (path.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("CSV path must not be empty."));
    }
    if (rows.size() > MaxCsvRows) {
        return ToolResult::failure(QStringLiteral("Too many rows: %1 (max %2)").arg(rows.size()).arg(MaxCsvRows));
    }

    // 校验列数一致
    int columnCount = -1;
    if (!header.isEmpty()) {
        columnCount = header.size();
    }
    for (int i = 0; i < rows.size(); ++i) {
        const int rowCols = rows.at(i).size();
        if (columnCount < 0) {
            columnCount = rowCols;
        } else if (rowCols != columnCount) {
            return ToolResult::failure(
                QStringLiteral("Column count mismatch: row %1 has %2 columns, expected %3")
                    .arg(i + 1).arg(rowCols).arg(columnCount));
        }
    }
    if (columnCount <= 0) {
        return ToolResult::failure(QStringLiteral("CSV must have at least one column."));
    }

    const QString resolvedPath = QDir(workspaceDirectory).filePath(path);
    if (!WorkspacePolicy::isPathInsideWorkspace(workspaceDirectory, resolvedPath)) {
        return ToolResult::failure(QStringLiteral("Path is outside workspace directory."));
    }

    QFile file(resolvedPath);
    if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        return ToolResult::failure(QStringLiteral("Cannot write CSV file: %1").arg(file.errorString()));
    }

    QTextStream stream(&file);

    // 写入表头
    if (!header.isEmpty()) {
        QStringList escapedHeader;
        for (const QString &col : header) {
            escapedHeader.append(escapeCsvField(col));
        }
        stream << escapedHeader.join(QLatin1Char(',')) << QStringLiteral("\r\n");
    }

    // 写入数据行
    for (const QStringList &row : rows) {
        QStringList escapedRow;
        for (const QString &field : row) {
            escapedRow.append(escapeCsvField(field));
        }
        stream << escapedRow.join(QLatin1Char(',')) << QStringLiteral("\r\n");
    }

    file.close();
    return ToolResult::success(QStringLiteral("CSV written: %1 (%2 rows, %3 columns)")
                                   .arg(path)
                                   .arg(rows.size())
                                   .arg(columnCount));
}

} // namespace CsvDataService
