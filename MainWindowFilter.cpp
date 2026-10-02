// =============================================================================
// MainWindowFilter.cpp —— 筛选板块
// 负责筛选字段下拉框、条件列表的维护，以及依据勾选条件计算抽取候选集合
// （drawCandidates）。候选集合同时供左侧人员列表显示与随机抽取使用。
// =============================================================================

#include "MainWindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QSet>
#include <QStandardItem>
#include <QStandardItemModel>

void MainWindow::updateFilterFields()
{
    auto *model = qobject_cast<QStandardItemModel *>(
        filterTypeComboBox->model());

    if (!model) {
        model = new QStandardItemModel(filterTypeComboBox);
        filterTypeComboBox->setModel(model);
    }

    model->blockSignals(true);
    model->clear();

    auto *allItem = new QStandardItem("未选择字段（全部）");
    allItem->setFlags(Qt::ItemIsEnabled);
    model->appendRow(allItem);

    QStringList fields;
    fields << "姓名";

    if (currentListIndex >= 0 &&
        currentListIndex < nameLists.size()) {
        for (const Person &person :
             nameLists[currentListIndex].people) {
            for (auto it = person.information.cbegin();
                 it != person.information.cend();
                 ++it) {
                if (!fields.contains(it.key()))
                    fields.append(it.key());
            }
        }
    }

    for (const QString &field : fields) {
        auto *item = new QStandardItem(field);
        item->setFlags(Qt::ItemIsEnabled |
                       Qt::ItemIsUserCheckable);
        item->setData(field, Qt::UserRole);
        item->setCheckState(Qt::Unchecked);
        model->appendRow(item);
    }

    model->blockSignals(false);
    filterTypeComboBox->setCurrentIndex(0);
    updateFilterSummary();
    updateFilterConditions();
}

QStringList MainWindow::selectedFilterFields() const
{
    QStringList result;

    auto *model = qobject_cast<QStandardItemModel *>(
        filterTypeComboBox->model());

    if (!model)
        return result;

    for (int i = 1; i < model->rowCount(); ++i) {
        const QStandardItem *item = model->item(i);
        if (item->checkState() == Qt::Checked)
            result.append(item->data(Qt::UserRole).toString());
    }

    return result;
}

void MainWindow::updateFilterSummary()
{
    const QStringList fields = selectedFilterFields();

    const QString text = fields.isEmpty()
        ? "未选择字段（全部）"
        : fields.join("、");

    filterTypeComboBox->setItemText(0, text);
    filterTypeComboBox->setToolTip(
        fields.isEmpty()
            ? "当前未选择字段：全部人员参与抽取"
            : "当前选择字段：" + fields.join("、"));
}

void MainWindow::updateFilterConditions()
{
    filterConditionList->blockSignals(true);
    filterConditionList->clear();

    const QStringList fields = selectedFilterFields();

    if (fields.isEmpty()) {
        filterConditionList->blockSignals(false);
        return;
    }

    QSet<QString> seen;

    for (const QString &field : fields) {
        for (const Person &person :
             nameLists[currentListIndex].people) {
            QString value;
            if (field == "姓名")
                value = person.name;
            else
                value = person.information.value(field);

            const QString key = field + QChar(0x1f) + value;
            if (seen.contains(key))
                continue;

            seen.insert(key);

            auto *item = new QListWidgetItem(
                field + "：" +
                (value.isEmpty() ? "（空）" : value),
                filterConditionList);
            item->setFlags(item->flags() |
                           Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
            item->setData(Qt::UserRole, field);
            item->setData(Qt::UserRole + 1, value);
        }
    }

    filterConditionList->blockSignals(false);
}

void MainWindow::selectAllConditions()
{
    for (int i = 0; i < filterConditionList->count(); ++i)
        filterConditionList->item(i)->setCheckState(Qt::Checked);
    filterChanged();
}

void MainWindow::clearConditions()
{
    for (int i = 0; i < filterConditionList->count(); ++i)
        filterConditionList->item(i)->setCheckState(Qt::Unchecked);
    filterChanged();
}

void MainWindow::invertConditions()
{
    for (int i = 0; i < filterConditionList->count(); ++i) {
        auto *item = filterConditionList->item(i);
        item->setCheckState(
            item->checkState() == Qt::Checked
                ? Qt::Unchecked
                : Qt::Checked);
    }
    filterChanged();
}

void MainWindow::filterChanged()
{
    if (currentListIndex < 0 ||
        currentListIndex >= nameLists.size())
        return;

    const QStringList fields = selectedFilterFields();

    QSet<QString> checkedConditions;
    for (int i = 0; i < filterConditionList->count(); ++i) {
        const QListWidgetItem *item =
            filterConditionList->item(i);
        if (item->checkState() == Qt::Checked) {
            const QString key =
                item->data(Qt::UserRole).toString()
                + QChar(0x1f)
                + item->data(Qt::UserRole + 1).toString();
            checkedConditions.insert(key);
        }
    }

    drawCandidates.clear();

    for (int i = 0;
         i < nameLists[currentListIndex].people.size();
         ++i) {
        const Person &person =
            nameLists[currentListIndex].people[i];

        bool matched = fields.isEmpty();

        if (!fields.isEmpty()) {
            for (const QString &field : fields) {
                QString value =
                    field == "姓名"
                        ? person.name
                        : person.information.value(field);

                const QString key =
                    field + QChar(0x1f) + value;

                if (checkedConditions.contains(key)) {
                    matched = true;
                    break;
                }
            }
        }

        if (matched &&
            noRepeatCheckBox->isChecked() &&
            historyIndices.contains(i)) {
            matched = false;
        }

        personListWidget->item(i)->setHidden(!matched);

        if (matched)
            drawCandidates.append(i);
    }
}
