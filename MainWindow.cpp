#include "MainWindow.h"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QDialog>
#include <QDialogButtonBox>
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
#include <QSizePolicy>
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
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // 顶部：已加载名单标签 + 添加名单按钮
    auto *listBar = new QWidget(this);
    auto *listBarLayout = new QHBoxLayout(listBar);
    listBarLayout->setContentsMargins(0, 0, 0, 0);
    listBarLayout->setSpacing(6);

    listTabBar = new QTabBar(this);
    listTabBar->setTabsClosable(true);
    listTabBar->setUsesScrollButtons(true);
    listTabBar->setExpanding(false);
    listTabBar->setElideMode(Qt::ElideRight);
    listTabBar->setDocumentMode(true);

    addListButton = new QPushButton("+", this);
    addListButton->setFixedSize(38, 34);
    addListButton->setToolTip("加载名单");

    listBarLayout->addWidget(listTabBar, 1);
    listBarLayout->addWidget(addListButton);
    mainLayout->addWidget(listBar);

    mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->setChildrenCollapsible(false);
    mainSplitter->setHandleWidth(6);

    // 左侧：人员列表 + 添加人员
    auto *leftWidget = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    personListWidget = new QListWidget(this);
    personListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    personListWidget->setAlternatingRowColors(true);
    leftLayout->addWidget(personListWidget, 1);

    addPersonButton = new QPushButton("添加人员", this);
    addPersonButton->setMinimumHeight(40);
    leftLayout->addWidget(addPersonButton);

    // 右侧：上方人员信息，下方筛选 + 抽取
    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);

    auto *personInfoWidget = new QWidget(this);
    personInfoWidget->setObjectName("personInfoWidget");
    auto *infoLayout = new QHBoxLayout(personInfoWidget);
    infoLayout->setContentsMargins(28, 24, 28, 24);
    infoLayout->setSpacing(36);

    personNameLabel = new QLabel("请选择人员", this);
    personNameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    personNameLabel->setWordWrap(true);
    personNameLabel->setMinimumWidth(220);
    personNameLabel->setStyleSheet(
        "font-size: 36px; font-weight: 700;");

    informationWidget = new QWidget(this);
    auto *informationLayout = new QFormLayout(informationWidget);
    informationLayout->setContentsMargins(0, 0, 0, 0);
    informationLayout->setHorizontalSpacing(24);
    informationLayout->setVerticalSpacing(12);
    informationLayout->setFieldGrowthPolicy(
        QFormLayout::AllNonFixedFieldsGrow);
    informationLayout->setLabelAlignment(
        Qt::AlignRight | Qt::AlignVCenter);

    infoLayout->addWidget(personNameLabel, 2);
    infoLayout->addWidget(informationWidget, 3);
    rightLayout->addWidget(personInfoWidget, 7);

    auto *controlWidget = new QWidget(this);
    controlWidget->setObjectName("controlWidget");
    auto *controlLayout = new QHBoxLayout(controlWidget);
    controlLayout->setContentsMargins(22, 18, 22, 18);
    controlLayout->setSpacing(20);

    // 筛选条件区域
    auto *filterWidget = new QWidget(this);
    auto *filterLayout = new QVBoxLayout(filterWidget);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout->setSpacing(8);

    auto *filterTitle = new QLabel("筛选条件", this);
    filterTitle->setStyleSheet(
        "font-size: 17px; font-weight: 600;");

    auto *filterRow = new QHBoxLayout();
    filterRow->setSpacing(8);

    filterTypeComboBox = new QComboBox(this);
    filterTypeComboBox->setMinimumHeight(38);
    filterTypeComboBox->setMinimumWidth(180);
    filterTypeComboBox->setToolTip(
        "可多选字段；不选择字段时搜索全部字段");

    filterEdit = new QLineEdit(this);
    filterEdit->setMinimumHeight(38);
    filterEdit->setPlaceholderText(
        "输入关键词");

    filterRow->addWidget(filterTypeComboBox, 0);
    filterRow->addWidget(filterEdit, 1);

    auto *filterHint = new QLabel(
        "可复选多个字段；不选择字段时在全部字段中抽取",
        this);
    filterHint->setObjectName("filterHint");
    filterHint->setWordWrap(true);

    filterLayout->addWidget(filterTitle);
    filterLayout->addLayout(filterRow);
    filterLayout->addWidget(filterHint);
    filterLayout->addStretch();

    // 抽取按钮区域
    auto *drawArea = new QWidget(this);
    auto *drawLayout = new QVBoxLayout(drawArea);
    drawLayout->setContentsMargins(0, 0, 0, 0);

    drawButton = new QPushButton("抽取", this);
    drawButton->setMinimumSize(170, 86);
    drawButton->setSizePolicy(
        QSizePolicy::Preferred, QSizePolicy::Expanding);
    drawButton->setObjectName("drawButton");

    drawLayout->addStretch();
    drawLayout->addWidget(drawButton);
    drawLayout->addStretch();

    controlLayout->addWidget(filterWidget, 1);
    controlLayout->addWidget(drawArea, 0);

    rightLayout->addWidget(controlWidget, 3);

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
    connect(addPersonButton, &QPushButton::clicked,
            this, &MainWindow::addPerson);
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
        qApp->setStyleSheet(R"(
            QMainWindow {
                background: #f5f6f8;
            }
            QTabBar::tab {
                background: #e8eaee;
                border: 1px solid #d5d8de;
                border-bottom: none;
                padding: 8px 16px;
                margin-right: 2px;
                min-width: 90px;
            }
            QTabBar::tab:selected {
                background: #ffffff;
                font-weight: 600;
            }
            QListWidget, #personInfoWidget, #controlWidget {
                background: #ffffff;
                border: 1px solid #e0e2e7;
                border-radius: 8px;
            }
            QListWidget::item {
                padding: 8px 10px;
            }
            QListWidget::item:selected {
                background: #e9eefc;
                color: #1f2937;
            }
            QLineEdit, QComboBox {
                background: #ffffff;
                border: 1px solid #cfd3da;
                border-radius: 6px;
                padding: 5px 9px;
            }
            QLineEdit:focus, QComboBox:focus {
                border: 1px solid #7b8ed6;
            }
            QPushButton {
                background: #ffffff;
                border: 1px solid #cfd3da;
                border-radius: 7px;
                padding: 7px 14px;
            }
            QPushButton:hover {
                background: #f1f3f6;
            }
            #drawButton {
                font-size: 24px;
                font-weight: 700;
            }
            #drawButton:hover {
                background: #eef2ff;
            }
            #filterHint {
                color: #737985;
            }
        )");
        return;
    }

    qApp->setStyleSheet(R"(
        QMainWindow {
            background: #202124;
        }
        QMenuBar, QMenu {
            background: #202124;
            color: #e8eaed;
        }
        QMenu::item:selected {
            background: #3c4043;
        }
        QTabBar::tab {
            background: #292a2d;
            color: #e8eaed;
            border: 1px solid #3d4044;
            border-bottom: none;
            padding: 8px 16px;
            margin-right: 2px;
            min-width: 90px;
        }
        QTabBar::tab:selected {
            background: #3c4043;
            font-weight: 600;
        }
        QListWidget, #personInfoWidget, #controlWidget {
            background: #292a2d;
            border: 1px solid #45474c;
            border-radius: 8px;
            color: #e8eaed;
        }
        QListWidget::item {
            padding: 8px 10px;
        }
        QListWidget::item:selected {
            background: #3c4043;
            color: #ffffff;
        }
        QLineEdit, QComboBox {
            background: #202124;
            color: #e8eaed;
            border: 1px solid #55585e;
            border-radius: 6px;
            padding: 5px 9px;
        }
        QPushButton {
            background: #303134;
            color: #e8eaed;
            border: 1px solid #55585e;
            border-radius: 7px;
            padding: 7px 14px;
        }
        QPushButton:hover {
            background: #3c4043;
        }
        #drawButton {
            font-size: 24px;
            font-weight: 700;
        }
        #filterHint {
            color: #9aa0a6;
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
            header = header;

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
