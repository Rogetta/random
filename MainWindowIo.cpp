// =============================================================================
// MainWindowIo.cpp —— 序列化与文件板块
// 负责名单的序列化（Markdown / CSV）、反序列化（Markdown / CSV / TXT），
// 以及保存、另存和“自动加载上一次名单”所需的路径记录。
// Markdown 表格的解析细节依赖 MarkdownTable.h。
// =============================================================================

#include "MainWindow.h"
#include "MarkdownTable.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include <QTabBar>

QString MainWindow::serializeList(const NameList &list) const
{
    const QString suffix =
        QFileInfo(list.filePath)
            .suffix().toLower();

    if (suffix == "md")
        return serializeMarkdown(list);

    return serializeCsv(list);
}

QString MainWindow::serializeMarkdown(const NameList &list) const
{
    QStringList fields;
    fields << "姓名";

    for (const Person &person : list.people) {
        for (auto it = person.information.cbegin();
             it != person.information.cend();
             ++it) {
            if (!fields.contains(it.key()))
                fields.append(it.key());
        }
    }

    QString out;
    out += "| ";

    for (const QString &field : fields)
        out += MarkdownTable::escapeCell(field) + " | ";

    out += "\n| ";

    for (int i = 0; i < fields.size(); ++i)
        out += "--- | ";

    out += "\n";

    for (const Person &person : list.people) {
        out += "| ";
        out += MarkdownTable::escapeCell(person.name);
        out += " | ";

        for (int i = 1; i < fields.size(); ++i) {
            out += MarkdownTable::escapeCell(
                person.information.value(fields[i]));
            out += " | ";
        }

        out += "\n";
    }

    return out;
}

QString MainWindow::serializeCsv(const NameList &list) const
{
    QStringList fields;
    fields << "姓名";

    for (const Person &person : list.people) {
        for (auto it = person.information.cbegin();
             it != person.information.cend();
             ++it) {
            if (!fields.contains(it.key()))
                fields.append(it.key());
        }
    }

    QString out;

    for (int i = 0; i < fields.size(); ++i) {
        if (i > 0)
            out += ",";
        out += fields[i];
    }
    out += "\n";

    for (const Person &person : list.people) {
        for (int i = 0; i < fields.size(); ++i) {
            if (i > 0)
                out += ",";

            if (i == 0)
                out += person.name;
            else
                out += person.information.value(fields[i]);
        }
        out += "\n";
    }

    return out;
}

bool MainWindow::deserializeList(
    const QString &path,
    NameList &list)
{
    QFile file(path);

    if (!file.open(
            QIODevice::ReadOnly |
            QIODevice::Text))
        return false;

    const QString text =
        QString::fromUtf8(file.readAll());

    const QStringList lines =
        text.split(
            QRegularExpression("[\\r\\n]+"),
            Qt::SkipEmptyParts);

    if (lines.isEmpty())
        return false;

    list = NameList{};
    list.filePath = path;
    list.name =
        QFileInfo(path).completeBaseName();

    const QString suffix =
        QFileInfo(path).suffix().toLower();

    bool markdown =
        suffix == "md" ||
        lines.first().trimmed().startsWith('|');

    if (markdown) {
        if (lines.size() < 2 ||
            !MarkdownTable::isSeparator(lines.at(1)))
            return false;

        QStringList headers =
            MarkdownTable::splitRow(lines.first());

        for (QString &header : headers)
            header = header;

        if (headers.isEmpty() ||
            headers.first() != "姓名")
            return false;

        for (int row = 2;
             row < lines.size();
             ++row) {
            QStringList values =
                MarkdownTable::splitRow(lines.at(row));

            if (values.isEmpty())
                continue;

            while (values.size() < headers.size())
                values.append(QString());

            Person person;
            person.name =
                MarkdownTable::unescapeCell(
                    values.first());

            if (person.name.isEmpty())
                continue;

            for (int i = 1;
                 i < headers.size();
                 ++i) {
                const QString key =
                    headers.at(i).trimmed();

                if (key.isEmpty())
                    continue;

                person.information.insert(
                    key,
                    i < values.size()
                        ? MarkdownTable::unescapeCell(
                              values.at(i))
                        : QString());
            }

            list.people.append(person);
        }

        return !list.people.isEmpty();
    }

    const QStringList headers =
        lines.first().split(
            ",", Qt::KeepEmptyParts);

    if (headers.isEmpty() ||
        headers.first().trimmed() != "姓名")
        return false;

    for (int row = 1;
         row < lines.size();
         ++row) {
        const QStringList values =
            lines.at(row).split(
                ",", Qt::KeepEmptyParts);

        if (values.isEmpty() ||
            values.first().trimmed().isEmpty())
            continue;

        Person person;
        person.name =
            values.first().trimmed();

        for (int i = 1;
             i < headers.size();
             ++i) {
            const QString key =
                headers.at(i).trimmed();

            if (key.isEmpty())
                continue;

            person.information.insert(
                key,
                i < values.size()
                    ? values.at(i).trimmed()
                    : QString());
        }

        list.people.append(person);
    }

    return !list.people.isEmpty();
}

void MainWindow::saveList()
{
    if (currentListIndex < 0 ||
        currentListIndex >= nameLists.size()) {
        QMessageBox::information(
            this,
            "保存名单",
            "当前没有打开的名单。");
        return;
    }

    NameList &list =
        nameLists[currentListIndex];

    if (list.filePath.isEmpty()) {
        saveListAs();
        return;
    }

    QFile file(list.filePath);

    if (!file.open(
            QIODevice::WriteOnly |
            QIODevice::Text)) {
        QMessageBox::warning(
            this,
            "保存失败",
            "无法写入文件。");
        return;
    }

    file.write(
        serializeList(list).toUtf8());
}

void MainWindow::saveListAs()
{
    if (currentListIndex < 0 ||
        currentListIndex >= nameLists.size())
        return;

    const QString path =
        QFileDialog::getSaveFileName(
            this,
            "另存名单",
            nameLists[currentListIndex].name +
                ".md",
            "Markdown 名单 (*.md);;"
            "CSV 名单 (*.csv);;"
            "文本文件 (*.txt)");

    if (path.isEmpty())
        return;

    NameList &list =
        nameLists[currentListIndex];

    list.filePath = path;
    list.name =
        QFileInfo(path).completeBaseName();

    QFile file(path);

    if (!file.open(
            QIODevice::WriteOnly |
            QIODevice::Text)) {
        QMessageBox::warning(
            this,
            "保存失败",
            "无法写入文件。");
        return;
    }

    file.write(
        serializeList(list).toUtf8());

    listTabBar->setTabText(
        currentListIndex,
        list.name);

    saveLastPath(path);
}

void MainWindow::saveLastPath(
    const QString &path)
{
    QSettings().setValue(
        "lastListPath", path);
}

void MainWindow::loadLastListIfEnabled()
{
    if (!autoLoadAction->isChecked())
        return;

    const QString path =
        QSettings()
            .value("lastListPath")
            .toString();

    if (path.isEmpty() ||
        !QFileInfo::exists(path))
        return;

    NameList list;

    if (deserializeList(path, list))
        addNameList(list);
}
