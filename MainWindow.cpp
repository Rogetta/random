#include "MainWindow.h"
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
#include <QTabBar>
#include <QTimer>
#include <QVBoxLayout>

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
    autoLoadAction->setChecked(settings.value("autoLoadLastList", false).toBool());
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

    auto *listBar = new QWidget(this);
    auto *listBarLayout = new QHBoxLayout(listBar);
    listBarLayout->setContentsMargins(0, 0, 0, 0);
    listTabBar = new QTabBar(this);
    listTabBar->setTabsClosable(true);
    listTabBar->setUsesScrollButtons(true);
    listTabBar->setExpanding(false);
    addListButton = new QPushButton("+", this);
    addListButton->setFixedSize(36, 30);
    listBarLayout->addWidget(listTabBar, 1);
    listBarLayout->addWidget(addListButton);
    mainLayout->addWidget(listBar);

    mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->setChildrenCollapsible(false);

    auto *leftWidget = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    personListWidget = new QListWidget(this);
    leftLayout->addWidget(personListWidget);

    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    auto *personInfoWidget = new QWidget(this);
    auto *infoLayout = new QHBoxLayout(personInfoWidget);
    infoLayout->setContentsMargins(20, 20, 20, 20);
    personNameLabel = new QLabel("请选择人员", this);
    personNameLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    personNameLabel->setStyleSheet("font-size: 32px; font-weight: bold;");
    informationWidget = new QWidget(this);
    auto *form = new QFormLayout(informationWidget);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
    infoLayout->addWidget(personNameLabel, 1);
    infoLayout->addWidget(informationWidget, 1);
    rightLayout->addWidget(personInfoWidget, 7);

    auto *bottom = new QWidget(this);
    auto *bottomLayout = new QHBoxLayout(bottom);
    auto *filterWidget = new QWidget(this);
    auto *filterLayout = new QHBoxLayout(filterWidget);
    filterTypeComboBox = new QComboBox(this);
    filterTypeComboBox->addItem("全部");
    filterEdit = new QLineEdit(this);
    filterEdit->setPlaceholderText("输入筛选内容");
    filterLayout->addWidget(new QLabel("筛选：", this));
    filterLayout->addWidget(filterTypeComboBox);
    filterLayout->addWidget(filterEdit, 1);
    drawButton = new QPushButton("抽取", this);
    drawButton->setFixedSize(120, 50);
    bottomLayout->addWidget(filterWidget, 1);
    bottomLayout->addWidget(drawButton);
    rightLayout->addWidget(bottom, 3);

    mainSplitter->addWidget(leftWidget);
    mainSplitter->addWidget(rightWidget);
    mainSplitter->setStretchFactor(0, 2);
    mainSplitter->setStretchFactor(1, 8);
    mainLayout->addWidget(mainSplitter, 1);

    drawTimer = new QTimer(this);
    drawTimer->setInterval(70);
}

void MainWindow::setupConnections()
{
    connect(loadAction, &QAction::triggered, this, &MainWindow::loadList);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveList);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveListAs);
    connect(addListButton, &QPushButton::clicked, this, &MainWindow::addList);
    connect(listTabBar, &QTabBar::tabCloseRequested, this, &MainWindow::closeList);
    connect(listTabBar, &QTabBar::currentChanged, this, &MainWindow::currentListChanged);
    connect(personListWidget, &QListWidget::itemClicked, this, &MainWindow::personSelected);
    connect(drawButton, &QPushButton::clicked, this, &MainWindow::toggleDraw);
    connect(drawTimer, &QTimer::timeout, this, &MainWindow::drawStep);
    connect(filterTypeComboBox, &QComboBox::currentTextChanged, this, &MainWindow::filterChanged);
    connect(filterEdit, &QLineEdit::textChanged, this, &MainWindow::filterChanged);
    connect(autoLoadAction, &QAction::toggled, this, &MainWindow::autoLoadLastListChanged);
    connect(themeAction, &QAction::triggered, this, &MainWindow::toggleTheme);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::addList() { loadList(); }

void MainWindow::loadList()
{
    const QString path = QFileDialog::getOpenFileName(this, "加载名单", {}, "CSV/TXT (*.csv *.txt);;所有文件 (*.*)");
    if (path.isEmpty()) return;
    NameList list;
    if (!deserializeList(path, list)) {
        QMessageBox::warning(this, "加载失败", "无法读取名单文件，第一列必须是“姓名”。");
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
    if (index < 0 || index >= nameLists.size()) return;
    if (drawing) toggleDraw();
    nameLists.removeAt(index);
    listTabBar->removeTab(index);
    currentListIndex = listTabBar->currentIndex();
    updatePersonList();
}

void MainWindow::currentListChanged(int index)
{
    if (drawing) toggleDraw();
    currentListIndex = index;
    updatePersonList();
}

void MainWindow::updateFilterFields()
{
    filterTypeComboBox->blockSignals(true);
    filterTypeComboBox->clear();
    filterTypeComboBox->addItem("全部");
    if (currentListIndex >= 0 && currentListIndex < nameLists.size()) {
        QSet<QString> fields;
        for (const auto &p : nameLists[currentListIndex].people)
            for (auto it = p.information.cbegin(); it != p.information.cend(); ++it)
                fields.insert(it.key());
        for (const auto &field : fields) filterTypeComboBox->addItem(field);
    }
    filterTypeComboBox->blockSignals(false);
}

void MainWindow::updatePersonList()
{
    personListWidget->clear();
    updateFilterFields();
    personNameLabel->setText("请选择人员");
    if (currentListIndex < 0 || currentListIndex >= nameLists.size()) return;
    for (const auto &p : nameLists[currentListIndex].people)
        personListWidget->addItem(p.name);
    filterChanged();
}

void MainWindow::personSelected(QListWidgetItem *item)
{
    if (!item || currentListIndex < 0 || currentListIndex >= nameLists.size()) return;
    for (const auto &p : nameLists[currentListIndex].people)
        if (p.name == item->text()) { updatePersonInformation(p); return; }
}

void MainWindow::updatePersonInformation(const Person &person)
{
    personNameLabel->setText(person.name);
    auto *layout = qobject_cast<QFormLayout *>(informationWidget->layout());
    while (auto *item = layout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    for (auto it = person.information.cbegin(); it != person.information.cend(); ++it) {
        auto *value = new QLabel(it.value(), informationWidget);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        value->setWordWrap(true);
        layout->addRow(it.key() + "：", value);
    }
}

void MainWindow::filterChanged()
{
    if (currentListIndex < 0 || currentListIndex >= nameLists.size()) return;
    const QString type = filterTypeComboBox->currentText();
    const QString key = filterEdit->text().trimmed();
    drawCandidates.clear();

    for (int i = 0; i < nameLists[currentListIndex].people.size(); ++i) {
        const auto &p = nameLists[currentListIndex].people[i];
        bool matched = key.isEmpty();
        if (!matched && type == "全部") {
            matched = p.name.contains(key, Qt::CaseInsensitive);
            for (auto it = p.information.cbegin(); it != p.information.cend() && !matched; ++it)
                matched = it.value().contains(key, Qt::CaseInsensitive);
        } else if (!matched) {
            matched = p.information.value(type).contains(key, Qt::CaseInsensitive);
        }
        personListWidget->item(i)->setHidden(!matched);
        if (matched) drawCandidates.append(i);
    }
}

void MainWindow::toggleDraw()
{
    if (drawing) {
        drawing = false;
        drawTimer->stop();
        drawButton->setText("抽取");
        if (currentDrawIndex >= 0 && currentListIndex >= 0)
            updatePersonInformation(nameLists[currentListIndex].people[currentDrawIndex]);
        return;
    }
    filterChanged();
    if (drawCandidates.isEmpty()) {
        QMessageBox::information(this, "无法抽取", "当前没有符合筛选条件的人员。");
        return;
    }
    drawing = true;
    drawButton->setText("停止");
    drawTimer->start();
}

void MainWindow::drawStep()
{
    if (drawCandidates.isEmpty()) return;
    currentDrawIndex = drawCandidates.at(QRandomGenerator::global()->bounded(drawCandidates.size()));
    updatePersonInformation(nameLists[currentListIndex].people[currentDrawIndex]);
}

void MainWindow::autoLoadLastListChanged(bool checked)
{
    QSettings().setValue("autoLoadLastList", checked);
}

void MainWindow::toggleTheme()
{
    darkTheme = !darkTheme;
    QSettings().setValue("darkTheme", darkTheme);
    applyTheme();
}

void MainWindow::applyTheme()
{
    if (!darkTheme) { qApp->setStyleSheet(QString()); return; }
    qApp->setStyleSheet(R"(
        QWidget { background:#202124; color:#e8eaed; }
        QMenuBar,QMenu { background:#202124; color:#e8eaed; }
        QMenu::item:selected { background:#3c4043; }
        QListWidget,QLineEdit,QComboBox { background:#292a2d; border:1px solid #555; }
        QListWidget::item:selected { background:#3c4043; }
        QPushButton { background:#303134; border:1px solid #555; padding:7px 12px; }
        QPushButton:hover { background:#3c4043; }
        QTabBar::tab { background:#292a2d; padding:7px 14px; }
        QTabBar::tab:selected { background:#3c4043; }
    )");
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "关于名单抽取",
        "<h3>名单抽取</h3><p>版本：1.0.0</p><p>作者：Rogetta</p><p>Qt 6 / Qt Widgets</p>");
}

QString MainWindow::serializeList(const NameList &list) const
{
    QString out = "姓名";
    QSet<QString> fields;
    for (const auto &p : list.people)
        for (auto it = p.information.cbegin(); it != p.information.cend(); ++it)
            fields.insert(it.key());
    const auto names = fields.values();
    for (const auto &f : names) out += "," + f;
    out += "\n";
    for (const auto &p : list.people) {
        out += p.name;
        for (const auto &f : names) out += "," + p.information.value(f);
        out += "\n";
    }
    return out;
}

bool MainWindow::deserializeList(const QString &path, NameList &list)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    const QString text = QString::fromUtf8(file.readAll());
    const auto lines = text.split(QRegularExpression("[\\r\\n]+"), Qt::SkipEmptyParts);
    if (lines.isEmpty()) return false;
    const auto headers = lines.first().split(",", Qt::KeepEmptyParts);
    if (headers.isEmpty() || headers.first().trimmed() != "姓名") return false;
    list.filePath = path;
    list.name = QFileInfo(path).completeBaseName();
    for (int row = 1; row < lines.size(); ++row) {
        const auto values = lines.at(row).split(",", Qt::KeepEmptyParts);
        if (values.isEmpty() || values.first().trimmed().isEmpty()) continue;
        Person p;
        p.name = values.first().trimmed();
        for (int i = 1; i < headers.size(); ++i)
            p.information.insert(headers.at(i).trimmed(), i < values.size() ? values.at(i).trimmed() : QString());
        list.people.append(p);
    }
    return !list.people.isEmpty();
}

void MainWindow::saveList()
{
    if (currentListIndex < 0 || currentListIndex >= nameLists.size()) return saveListAs();
    const auto &list = nameLists[currentListIndex];
    if (list.filePath.isEmpty()) return saveListAs();
    QFile file(list.filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "保存失败", "无法写入文件。");
        return;
    }
    file.write(serializeList(list).toUtf8());
}

void MainWindow::saveListAs()
{
    if (currentListIndex < 0 || currentListIndex >= nameLists.size()) return;
    const QString path = QFileDialog::getSaveFileName(this, "另存名单",
        nameLists[currentListIndex].name + ".csv", "CSV 文件 (*.csv);;文本文件 (*.txt)");
    if (path.isEmpty()) return;
    auto &list = nameLists[currentListIndex];
    list.filePath = path;
    list.name = QFileInfo(path).completeBaseName();
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "保存失败", "无法写入文件。");
        return;
    }
    file.write(serializeList(list).toUtf8());
    listTabBar->setTabText(currentListIndex, list.name);
    saveLastPath(path);
}

void MainWindow::saveLastPath(const QString &path) { QSettings().setValue("lastListPath", path); }

void MainWindow::loadLastListIfEnabled()
{
    if (!autoLoadAction->isChecked()) return;
    const QString path = QSettings().value("lastListPath").toString();
    if (path.isEmpty() || !QFileInfo::exists(path)) return;
    NameList list;
    if (deserializeList(path, list)) addNameList(list);
}
