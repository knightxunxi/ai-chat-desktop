#pragma once

#include "tools/ToolResult.h"

#include <QString>
#include <QStringList>
#include <QVector>

namespace CsvDataService {

// 功能：从工作目录内的 CSV 文件读取数据。
// path 为相对路径，限定在工作目录内；maxRows 默认 500。
ToolResult readCsv(const QString &workspaceDirectory,
                   const QString &path,
                   int maxRows = 500,
                   bool hasHeader = true);

// 功能：将二维字符串数组写入工作目录内的 CSV 文件。
// 每行列数必须一致；path 为相对路径。
ToolResult writeCsv(const QString &workspaceDirectory,
                    const QString &path,
                    const QVector<QStringList> &rows,
                    const QStringList &header = QStringList());

} // namespace CsvDataService
