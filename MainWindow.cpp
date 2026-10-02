// =============================================================================
// MainWindow.cpp —— 生命周期与信号装配板块
// 负责 MainWindow 的构造、菜单栏搭建，以及所有控件与槽函数之间的 connect。
//   · 界面构建 ........ MainWindowUi.cpp
//   · 名单 / 人员 ...... MainWindowList.cpp
//   · 筛选 ............ MainWindowFilter.cpp
//   · 抽取 / 历史 ...... MainWindowDraw.cpp
//   · 序列化 / 文件 .... MainWindowIo.cpp
//   · Markdown 工具 .... MarkdownTable.cpp
// =============================================================================

#include "MainWindow.h"

#include <QAbstractItemView>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSettings>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTabBar>
#include <QTimer>

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
    noRepeatAction->setChecked(
        settings.value("noRepeatDraw", false).toBool());
    noRepeatCheckBox->setChecked(noRepeatAction->isChecked());

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
    noRepeatAction = settingsMenu->addAction("已经抽到的人员不再抽取");
    noRepeatAction->setCheckable(true);

    themeAction = settingsMenu->addAction("亮暗主题切换");

    aboutMenu = menuBar()->addMenu("关于");
    aboutAction = aboutMenu->addAction("关于本程序");
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

    connect(autoLoadAction, &QAction::toggled,
            this, &MainWindow::autoLoadLastListChanged);
    connect(noRepeatAction, &QAction::toggled,
            this, &MainWindow::setNoRepeat);
    connect(noRepeatCheckBox, &QCheckBox::toggled,
            this, &MainWindow::setNoRepeat);
    connect(themeAction, &QAction::triggered,
            this, &MainWindow::toggleTheme);
    connect(aboutAction, &QAction::triggered,
            this, &MainWindow::showAbout);

    connect(selectAllButton, &QPushButton::clicked,
            this, &MainWindow::selectAllConditions);
    connect(clearConditionsButton, &QPushButton::clicked,
            this, &MainWindow::clearConditions);
    connect(invertConditionsButton, &QPushButton::clicked,
            this, &MainWindow::invertConditions);

    connect(filterConditionList, &QListWidget::itemChanged,
            this, [this](QListWidgetItem *) {
                filterChanged();
            });

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
                updateFilterConditions();
                filterChanged();
            });

    connect(historyListWidget, &QListWidget::itemClicked,
            this, [this](QListWidgetItem *item) {
                if (!item || currentListIndex < 0 ||
                    currentListIndex >= nameLists.size())
                    return;

                const int index =
                    item->data(Qt::UserRole).toInt();

                if (index >= 0 &&
                    index < nameLists[currentListIndex].people.size()) {
                    currentDrawIndex = index;
                    updatePersonInformation(
                        nameLists[currentListIndex].people[index]);
                }
            });

    connect(clearHistoryButton, &QPushButton::clicked,
            this, &MainWindow::clearHistory);
}
