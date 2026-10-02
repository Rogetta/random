// =============================================================================
// MainWindowList.cpp —— 名单与人员板块
// 负责名单的加载 / 添加 / 关闭与标签页切换，人员列表与详细信息的显示，
// 以及“添加人员”对话框。
// =============================================================================

#include "MainWindow.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QVBoxLayout>

void MainWindow::addPerson()
{
    if (currentListIndex < 0 ||
        currentListIndex >= nameLists.size()) {
        QMessageBox::information(
            this, "添加人员", "请先加载一个名单。");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("添加人员");
    dialog.setMinimumWidth(420);

    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout();
    form->setFieldGrowthPolicy(
        QFormLayout::AllNonFixedFieldsGrow);

    auto *nameEdit = new QLineEdit(&dialog);
    nameEdit->setPlaceholderText("请输入姓名");
    form->addRow("姓名：", nameEdit);

    QStringList fields;
    for (const Person &person : nameLists[currentListIndex].people) {
        for (auto it = person.information.cbegin();
             it != person.information.cend(); ++it) {
            if (!fields.contains(it.key()))
                fields.append(it.key());
        }
    }

    QVector<QLineEdit *> edits;
    for (const QString &field : fields) {
        auto *edit = new QLineEdit(&dialog);
        edits.append(edit);
        form->addRow(field + "：", edit);
    }

    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        &dialog);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted,
            &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected,
            &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString name = nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(
            this, "添加人员", "姓名不能为空。");
        return;
    }

    Person person;
    person.name = name;
    for (int i = 0; i < fields.size(); ++i)
        person.information.insert(
            fields.at(i), edits.at(i)->text().trimmed());

    nameLists[currentListIndex].people.append(person);
    updatePersonList();

    const int row = nameLists[currentListIndex].people.size() - 1;
    if (row >= 0 && row < personListWidget->count()) {
        personListWidget->setCurrentRow(row);
        updatePersonInformation(person);
    }
}

void MainWindow::addList()
{
    loadList();
}

void MainWindow::loadList()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        "加载名单",
        {},
        "名单文件 (*.md *.csv *.txt);;Markdown (*.md);;CSV (*.csv);;文本文件 (*.txt);;所有文件 (*.*)");

    if (path.isEmpty())
        return;

    NameList list;
    if (!deserializeList(path, list)) {
        QMessageBox::warning(
            this,
            "加载失败",
            "无法读取名单文件。\n\n"
            "Markdown 文件需要使用表格格式，并且第一列必须为“姓名”。");
        return;
    }

    addNameList(list);
    saveLastPath(path);
}

void MainWindow::addNameList(const NameList &list)
{
    nameLists.append(list);

    const int index = listTabBar->addTab(list.name);
    listTabBar->setCurrentIndex(index);
    currentListIndex = index;

    updatePersonList();
}

void MainWindow::closeList(int index)
{
    if (index < 0 || index >= nameLists.size())
        return;

    if (drawing)
        toggleDraw();

    nameLists.removeAt(index);
    listTabBar->removeTab(index);

    currentListIndex = listTabBar->currentIndex();
    updatePersonList();
}

void MainWindow::currentListChanged(int index)
{
    if (drawing)
        toggleDraw();

    currentListIndex = index;
    currentDrawIndex = -1;
    historyIndices.clear();
    historyListWidget->clear();
    historyCountLabel->setText("共0条记录");
    updatePersonList();
}

void MainWindow::updatePersonList()
{
    personListWidget->clear();
    updateFilterFields();
    personNameLabel->setText("请选择人员");

    if (currentListIndex < 0 ||
        currentListIndex >= nameLists.size())
        return;

    for (const Person &person :
         nameLists[currentListIndex].people) {
        personListWidget->addItem(person.name);
    }

    filterChanged();
}

void MainWindow::personSelected(QListWidgetItem *item)
{
    if (!item ||
        currentListIndex < 0 ||
        currentListIndex >= nameLists.size())
        return;

    const int row = personListWidget->row(item);
    if (row >= 0 &&
        row < nameLists[currentListIndex].people.size()) {
        currentDrawIndex = row;
        updatePersonInformation(
            nameLists[currentListIndex].people[row]);
    }
}

void MainWindow::editPerson()
{
    if (drawing ||
        currentListIndex < 0 ||
        currentListIndex >= nameLists.size())
        return;

    const int row = personListWidget->currentRow();
    if (row < 0 ||
        row >= nameLists[currentListIndex].people.size())
        return;

    Person &person = nameLists[currentListIndex].people[row];

    QDialog dialog(this);
    dialog.setWindowTitle("编辑人员");
    dialog.setMinimumWidth(420);

    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout();
    form->setFieldGrowthPolicy(
        QFormLayout::AllNonFixedFieldsGrow);

    auto *nameEdit = new QLineEdit(&dialog);
    nameEdit->setText(person.name);
    form->addRow("姓名：", nameEdit);

    QStringList fields;
    for (const Person &p : nameLists[currentListIndex].people) {
        for (auto it = p.information.cbegin();
             it != p.information.cend(); ++it) {
            if (!fields.contains(it.key()))
                fields.append(it.key());
        }
    }

    QVector<QLineEdit *> edits;
    for (const QString &field : fields) {
        auto *edit = new QLineEdit(&dialog);
        edit->setText(person.information.value(field));
        edits.append(edit);
        form->addRow(field + "：", edit);
    }

    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        &dialog);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted,
            &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected,
            &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString name = nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(
            this, "编辑人员", "姓名不能为空。");
        return;
    }

    person.name = name;
    person.information.clear();

    for (int i = 0; i < fields.size(); ++i)
        person.information.insert(
            fields.at(i),
            edits.at(i)->text().trimmed());

    updatePersonList();

    if (row < personListWidget->count()) {
        personListWidget->setCurrentRow(row);
        currentDrawIndex = row;
        updatePersonInformation(person);
    }
}

void MainWindow::updatePersonInformation(const Person &person)
{
    personNameLabel->setText(person.name);

    auto *layout =
        qobject_cast<QFormLayout *>(informationWidget->layout());

    while (QLayoutItem *item = layout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    for (auto it = person.information.cbegin();
         it != person.information.cend();
         ++it) {
        auto *value = new QLabel(
            it.value(), informationWidget);
        value->setTextInteractionFlags(
            Qt::TextSelectableByMouse);
        value->setWordWrap(true);
        layout->addRow(it.key() + "：", value);
    }
}
