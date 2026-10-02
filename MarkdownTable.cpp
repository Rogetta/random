// =============================================================================
// MarkdownTable.cpp —— Markdown 表格工具板块
// 见 MarkdownTable.h 的接口说明。
// =============================================================================

#include "MarkdownTable.h"

namespace MarkdownTable {

QStringList splitRow(const QString &line)
{
    QString row = line.trimmed();
    if (row.startsWith('|'))
        row.remove(0, 1);
    if (row.endsWith('|') && !row.endsWith("\\|"))
        row.chop(1);

    QStringList result;
    QString current;
    bool escaped = false;

    for (const QChar ch : row) {
        if (escaped) {
            current += ch;
            escaped = false;
        } else if (ch == '\\') {
            escaped = true;
        } else if (ch == '|') {
            result << current.trimmed();
            current.clear();
        } else {
            current += ch;
        }
    }

    if (escaped)
        current += '\\';

    result << current.trimmed();
    return result;
}

bool isSeparator(const QString &line)
{
    const QStringList cells = splitRow(line);
    if (cells.isEmpty())
        return false;

    for (const QString &cell : cells) {
        const QString value = cell.trimmed();
        if (value.isEmpty())
            return false;

        QString test = value;
        test.remove(':');
        if (test.isEmpty() || test.count('-') < 1)
            return false;

        for (const QChar ch : test) {
            if (ch != '-')
                return false;
        }
    }

    return true;
}

QString escapeCell(QString value)
{
    value.replace('\\', "\\\\");
    value.replace('|', "\\|");
    value.replace('\r', ' ');
    value.replace('\n', ' ');
    return value;
}

QString unescapeCell(QString value)
{
    QString result;
    bool escaped = false;

    for (const QChar ch : value) {
        if (escaped) {
            result += ch;
            escaped = false;
        } else if (ch == '\\') {
            escaped = true;
        } else {
            result += ch;
        }
    }

    if (escaped)
        result += '\\';

    return result.trimmed();
}

} // namespace MarkdownTable
