#include "MainWindow.h"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSettings>
#include <QSet>
#include <QSplitter>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTabBar>
#include <QTimer>
#include <QVBoxLayout>

namespace {

QStringList splitMarkdownRow(const QString &line)
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

bool isMarkdownSeparator(const QString &line)
{
    const QStringList cells = splitMarkdownRow(line);
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

QString escapeMarkdownCell(QString value)
{
    value.replace('\\', "\\\\");
    value.replace('|', "\\|");
    value.replace('\r', ' ');
    value.replace('\n', ' ');
    return value;
}

QString unescapeMarkdownCell(QString value)
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

}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("名单抽取");
    resize(1200, 750);
    setMinimumSize(900, 600);

    setupMenuBar();
    setupUi();
    setupConnections();

    QSettings settings;
    darkTheme = settings.value("darkTheme", false).toBool();
    autoLoadAction->setChecked(
        settings.value("autoLoadLastList", false).toBool());

    applyTheme();
    loadLastListIfEnabled();
}

void MainWindow::setupMenuBar()
{
    fileMenu = menuBar()->addMenu("文件");
    loadAction = fileMenu->addAction("加载名单");
    saveAction = fileMenu->addAction("保存名单");
    saveAsAction = fileMenu->addAction("另存名单");

    settingsMenu = menuBar()->addMenu("设置");
    autoLoadAction = settingsMenu->addAction("自动加载上一次的名单");
    autoLoadAction->setCheckable(true);
    themeAction = settingsMenu->addAction("亮暗主题切换");

    aboutMenu = menuBar()->addMenu("关于");
    aboutAction = aboutMenu->addAction("关于本程序");
}

void MainWindow::setupUi()
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);

    auto *listBar = new QWidget(this);
    auto *listBarLayout = new QHBoxLayout(listBar);
    listBarLayout->setContentsMargins(0, 0, 0, 0);

    listTabBar = new QTabBar(this);
    listTabBar->setTabsClosable(true);
    listTabBar->setUsesScrollButtons(true);
    listTabBar->setExpanding(false);
    listTabBar->setElideMode(Qt::ElideRight);

    addListButton = new QPushButton("+", this);
    addListButton->setFixedSize(36, 30);
    addListButton->setToolTip("加载名单");

    listBarLayout->addWidget(listTabBar, 1);
    listBarLayout->addWidget(addListButton);
    mainLayout->addWidget(listBar);

    mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->setChildrenCollapsible(false);

    auto *leftWidget = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    personListWidget = new QListWidget(this);
    personListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    leftLayout->addWidget(personListWidget);

    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(6);

    auto *personInfoWidget = new QWidget(this);
    auto *infoLayout = new QHBoxLayout(personInfoWidget);
    infoLayout->setContentsMargins(20, 20, 20, 20);
    infoLayout->setSpacing(20);

    personNameLabel = new QLabel("请选择人员", this);
    personNameLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    personNameLabel->setWordWrap(true);
    personNameLabel->setStyleSheet(
        "font-size: 32px; font-weight: bold;");

    informationWidget = new QWidget(this);
    auto *informationLayout = new QFormLayout(informationWidget);
    informationLayout->setFieldGrowthPolicy(
        QFormLayout::AllNonFixedFieldsGrow);
    informationLayout->setLabelAlignment(
        Qt::AlignRight | Qt::AlignTop);

    infoLayout->addWidget(personNameLabel, 1);
    infoLayout->addWidget(informationWidget, 1);
    rightLayout->addWidget(personInfoWidget, 7);

    auto *bottomWidget = new QWidget(this);
    auto *bottomLayout = new QHBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(10, 10, 10, 10);

    auto *filterWidget = new QWidget(this);
    auto *filterLayout = new QHBoxLayout(filterWidget);
    filterLayout->setContentsMargins(0, 0, 0, 0);

    filterLayout->addWidget(new QLabel("筛选字段：", this));

    filterTypeComboBox = new QComboBox(this);
    filterTypeComboBox->setMinimumWidth(180);
    filterTypeComboBox->setToolTip(
        "可多选字段；不选择字段时搜索全部字段");

    filterEdit = new QLineEdit(this);
    filterEdit->setPlaceholderText(
        "输入筛选内容；多个字段之间按“或”匹配");

    filterLayout->addWidget(filterTypeComboBox);
    filterLayout->addWidget(filterEdit, 1);

    drawButton = new QPushButton("抽取", this);
    drawButton->setFixedSize(120, 50);

    bottomLayout->addWidget(filterWidget, 1);
    bottomLayout->addWidget(drawButton, 0, Qt::AlignRight);

    rightLayout->addWidget(bottomWidget, 3);

    mainSplitter->addWidget(leftWidget);
    mainSplitter->addWidget(rightWidget);
    mainSplitter->setStretchFactor(0, 2);
    mainSplitter->setStretchFactor(1, 8);
    mainSplitter->setSizes({240, 960});

    mainLayout->addWidget(mainSplitter, 1);

    drawTimer = new QTimer(this);
    drawTimer->setInterval(70);
}

void MainWindow::setupConnections()
{
    connect(loadAction, &QAction::triggered,
            this, &MainWindow::loadList);
    connect(saveAction, &QAction::triggered,
            this, &MainWindow::saveList);
    connect(saveAsAction, &QAction::triggered,
            this, &MainWindow::saveListAs);

    connect(addListButton, &QPushButton::clicked,
            this, &MainWindow::addList);
    connect(listTabBar, &QTabBar::tabCloseRequested,
            this, &MainWindow::closeList);
    connect(listTabBar, &QTabBar::currentChanged,
            this, &MainWindow::currentListChanged);

    connect(personListWidget, &QListWidget::itemClicked,
            this, &MainWindow::personSelected);

    connect(drawButton, &QPushButton::clicked,
            this, &MainWindow::toggleDraw);
    connect(drawTimer, &QTimer::timeout,
            this, &MainWindow::drawStep);

    connect(filterEdit, &QLineEdit::textChanged,
            this, &MainWindow::filterChanged);

    connect(autoLoadAction, &QAction::toggled,
            this, &MainWindow::autoLoadLastListChanged);
    connect(themeAction, &QAction::triggered,
            this, &MainWindow::toggleTheme);
    connect(aboutAction, &QAction::triggered,
            this, &MainWindow::showAbout);

    connect(filterTypeComboBox->view(),
            &QAbstractItemView::pressed,
            this,
            [this](const QModelIndex &index) {
                if (!index.isValid())
                    return;

                auto *model =
                    qobject_cast<QStandardItemModel *>(
                        filterTypeComboBox->model());

                if (!model)
                    return;

                if (index.row() == 0) {
                    for (int i = 1; i < model->rowCount(); ++i)
                        model->item(i)->setCheckState(Qt::Unchecked);
                } else {
                    QStandardItem *item = model->item(index.row());
                    item->setCheckState(
                        item->checkState() == Qt::Checked
                            ? Qt::Unchecked
                            : Qt::Checked);
                }

                filterTypeComboBox->setCurrentIndex(0);
                updateFilterSummary();
                filterChanged();
            });
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
    updatePersonList();
}

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

    QString text;
    if (fields.isEmpty()) {
        text = "未选择字段（全部）";
    } else {
        text = fields.join("、");
    }

    filterTypeComboBox->setItemText(0, text);
    filterTypeComboBox->setToolTip(
        fields.isEmpty()
            ? "当前未选择字段：搜索全部字段"
            : "当前选择：" + fields.join("、"));
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

    for (const Person &person :
         nameLists[currentListIndex].people) {
        if (person.name == item->text()) {
            updatePersonInformation(person);
            return;
        }
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

void MainWindow::filterChanged()
{
    if (currentListIndex < 0 ||
        currentListIndex >= nameLists.size())
        return;

    const QString keyword =
        filterEdit->text().trimmed();

    const QStringList fields =
        selectedFilterFields();

    drawCandidates.clear();

    for (int i = 0;
         i < nameLists[currentListIndex].people.size();
         ++i) {
        const Person &person =
            nameLists[currentListIndex].people[i];

        bool matched = keyword.isEmpty();

        if (!matched) {
            if (fields.isEmpty()) {
                matched =
                    person.name.contains(
                        keyword, Qt::CaseInsensitive);

                for (auto it = person.information.cbegin();
                     it != person.information.cend() &&
                     !matched;
                     ++it) {
                    matched =
                        it.value().contains(
                            keyword, Qt::CaseInsensitive);
                }
            } else {
                for (const QString &field : fields) {
                    QString value;

                    if (field == "姓名")
                        value = person.name;
                    else
                        value =
                            person.information.value(field);

                    if (value.contains(
                            keyword,
                            Qt::CaseInsensitive)) {
                        matched = true;
                        break;
                    }
                }
            }
        }

        personListWidget->item(i)->setHidden(!matched);

        if (matched)
            drawCandidates.append(i);
    }
}

void MainWindow::toggleDraw()
{
    if (drawing) {
        drawing = false;
        drawTimer->stop();
        drawButton->setText("抽取");

        if (currentDrawIndex >= 0 &&
            currentListIndex >= 0 &&
            currentListIndex < nameLists.size() &&
            currentDrawIndex <
                nameLists[currentListIndex].people.size()) {
            updatePersonInformation(
                nameLists[currentListIndex]
                    .people[currentDrawIndex]);
        }

        return;
    }

    filterChanged();

    if (drawCandidates.isEmpty()) {
        QMessageBox::information(
            this,
            "无法抽取",
            "当前没有符合筛选条件的人员。");
        return;
    }

    drawing = true;
    drawButton->setText("停止");
    drawTimer->start();
}

void MainWindow::drawStep()
{
    if (drawCandidates.isEmpty())
        return;

    currentDrawIndex =
        drawCandidates.at(
            QRandomGenerator::global()->bounded(
                drawCandidates.size()));

    updatePersonInformation(
        nameLists[currentListIndex]
            .people[currentDrawIndex]);
}

void MainWindow::autoLoadLastListChanged(bool checked)
{
    QSettings().setValue(
        "autoLoadLastList", checked);
}

void MainWindow::toggleTheme()
{
    darkTheme = !darkTheme;
    QSettings().setValue(
        "darkTheme", darkTheme);
    applyTheme();
}

void MainWindow::applyTheme()
{
    if (!darkTheme) {
        qApp->setStyleSheet(QString());
        return;
    }

    qApp->setStyleSheet(R"(
        QWidget {
            background-color: #202124;
            color: #e8eaed;
        }
        QMenuBar, QMenu {
            background-color: #202124;
            color: #e8eaed;
        }
        QMenu::item:selected {
            background-color: #3c4043;
        }
        QListWidget, QLineEdit, QComboBox {
            background-color: #292a2d;
            border: 1px solid #555;
        }
        QListWidget::item:selected {
            background-color: #3c4043;
        }
        QComboBox QAbstractItemView {
            background-color: #292a2d;
            color: #e8eaed;
        }
        QPushButton {
            background-color: #303134;
            border: 1px solid #555;
            padding: 7px 12px;
        }
        QPushButton:hover {
            background-color: #3c4043;
        }
        QTabBar::tab {
            background-color: #292a2d;
            padding: 7px 14px;
        }
        QTabBar::tab:selected {
            background-color: #3c4043;
        }
    )");
}

void MainWindow::showAbout()
{
    QMessageBox::about(
        this,
        "关于名单抽取",
        "<h3>名单抽取</h3>"
        "<p>版本：1.1.0</p>"
        "<p>作者：Rogetta</p>"
        "<p>基于 Qt 6 / Qt Widgets 开发。</p>");
}

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
        out += escapeMarkdownCell(field) + " | ";

    out += "\n| ";

    for (int i = 0; i < fields.size(); ++i)
        out += "--- | ";

    out += "\n";

    for (const Person &person : list.people) {
        out += "| ";
        out += escapeMarkdownCell(person.name);
        out += " | ";

        for (int i = 1; i < fields.size(); ++i) {
            out += escapeMarkdownCell(
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
            !isMarkdownSeparator(lines.at(1)))
            return false;

        QStringList headers =
            splitMarkdownRow(lines.first());

        for (QString &header : headers)
            header = unescapeMarkdownCell(header);

        if (headers.isEmpty() ||
            headers.first() != "姓名")
            return false;

        for (int row = 2;
             row < lines.size();
             ++row) {
            QStringList values =
                splitMarkdownRow(lines.at(row));

            if (values.isEmpty())
                continue;

            while (values.size() < headers.size())
                values.append(QString());

            Person person;
            person.name =
                unescapeMarkdownCell(
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
                        ? unescapeMarkdownCell(
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
