#ifndef MARKDOWNTABLE_H
#define MARKDOWNTABLE_H

#include <QString>
#include <QStringList>

// =============================================================================
// MarkdownTable.h —— Markdown 表格工具板块
// 提供 Markdown 表格行的拆分、分隔行识别，以及单元格的转义 / 反转义。
// 全部为无状态自由函数，供名单的读写（MainWindowIo.cpp）使用。
// =============================================================================

namespace MarkdownTable {

// 按未转义的 `|` 拆分一行表格，去掉首尾竖线并做去空白处理。
QStringList splitRow(const QString &line);

// 判断一行是否为 Markdown 表格的分隔行（如 `| --- | --- |`）。
bool isSeparator(const QString &line);

// 转义单元格中的反斜杠、竖线与换行，便于写入表格。
QString escapeCell(QString value);

// 还原 escapeCell 的转义，并去掉首尾空白。
QString unescapeCell(QString value);

} // namespace MarkdownTable

#endif // MARKDOWNTABLE_H
