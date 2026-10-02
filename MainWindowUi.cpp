// =============================================================================
// MainWindowUi.cpp —— 界面板块
// 负责主界面所有控件的创建与布局（setupUi）、亮暗主题切换（toggleTheme /
// applyTheme）以及“关于”对话框（showAbout）。
// =============================================================================

#include "MainWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QTabBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

void MainWindow::setupUi()
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

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

    // 左侧：名单 + 记录数量 + 添加人员
    auto *leftWidget = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    personListWidget = new QListWidget(this);
    personListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    personListWidget->setAlternatingRowColors(true);
    leftLayout->addWidget(personListWidget, 1);

    historyCountLabel = new QLabel("共0条记录", this);
    historyCountLabel->setObjectName("historyCountLabel");
    leftLayout->addWidget(historyCountLabel);

    auto *personButtonRow = new QWidget(this);
    auto *personButtonLayout = new QHBoxLayout(personButtonRow);
    personButtonLayout->setContentsMargins(0, 0, 0, 0);
    personButtonLayout->setSpacing(6);

    addPersonButton = new QPushButton("添加人员", this);
    addPersonButton->setMinimumHeight(40);

    editPersonButton = new QPushButton("编辑人员", this);
    editPersonButton->setMinimumHeight(40);
    editPersonButton->setEnabled(false);

    personButtonLayout->addWidget(addPersonButton);
    personButtonLayout->addWidget(editPersonButton);
    leftLayout->addWidget(personButtonRow);

    // 右侧
    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);

    // 上半部分：当前人员信息 + 历史抽取记录
    auto *personInfoArea = new QWidget(this);
    personInfoArea->setObjectName("personInfoArea");
    auto *personInfoLayout = new QHBoxLayout(personInfoArea);
    personInfoLayout->setContentsMargins(0, 0, 0, 0);
    personInfoLayout->setSpacing(10);

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

    auto *historyWidget = new QWidget(this);
    historyWidget->setObjectName("historyWidget");
    auto *historyLayout = new QVBoxLayout(historyWidget);
    historyLayout->setContentsMargins(16, 16, 16, 12);
    historyLayout->setSpacing(8);

    auto *historyTitle = new QLabel("历史抽取", this);
    historyTitle->setStyleSheet(
        "font-size: 17px; font-weight: 600;");

    historyListWidget = new QListWidget(this);
    historyListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    historyListWidget->setAlternatingRowColors(true);

    clearHistoryButton = new QPushButton("清除记录", this);
    clearHistoryButton->setMinimumHeight(34);

    historyLayout->addWidget(historyTitle);
    historyLayout->addWidget(historyListWidget, 1);
    historyLayout->addWidget(clearHistoryButton);

    personInfoLayout->addWidget(personInfoWidget, 3);
    personInfoLayout->addWidget(historyWidget, 2);
    rightLayout->addWidget(personInfoArea, 7);

    // 下半部分：筛选条件 + 抽取控制
    auto *controlWidget = new QWidget(this);
    controlWidget->setObjectName("controlWidget");
    auto *controlLayout = new QHBoxLayout(controlWidget);
    controlLayout->setContentsMargins(22, 18, 22, 18);
    controlLayout->setSpacing(20);

    auto *filterWidget = new QWidget(this);
    auto *filterLayout = new QVBoxLayout(filterWidget);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout->setSpacing(7);

    auto *filterTitle = new QLabel("筛选条件", this);
    filterTitle->setStyleSheet(
        "font-size: 17px; font-weight: 600;");

    auto *fieldRow = new QHBoxLayout();
    fieldRow->setSpacing(8);

    filterTypeComboBox = new QComboBox(this);
    filterTypeComboBox->setMinimumHeight(36);
    filterTypeComboBox->setMinimumWidth(190);
    filterTypeComboBox->setToolTip(
        "可多选字段；选择后下方列出该字段的所有条件");

    fieldRow->addWidget(new QLabel("字段：", this));
    fieldRow->addWidget(filterTypeComboBox, 1);

    filterConditionList = new QListWidget(this);
    filterConditionList->setMinimumHeight(105);
    filterConditionList->setMaximumHeight(180);
    filterConditionList->setSelectionMode(QAbstractItemView::NoSelection);

    auto *conditionButtons = new QHBoxLayout();
    conditionButtons->setSpacing(6);

    selectAllButton = new QPushButton("全选", this);
    clearConditionsButton = new QPushButton("清除", this);
    invertConditionsButton = new QPushButton("反选", this);

    conditionButtons->addWidget(selectAllButton);
    conditionButtons->addWidget(clearConditionsButton);
    conditionButtons->addWidget(invertConditionsButton);
    conditionButtons->addStretch();

    auto *filterHint = new QLabel(
        "选择字段后显示所有可用条件，默认全部选中；多个条件按“或”匹配",
        this);
    filterHint->setObjectName("filterHint");
    filterHint->setWordWrap(true);

    filterLayout->addWidget(filterTitle);
    filterLayout->addLayout(fieldRow);
    filterLayout->addWidget(filterConditionList, 1);
    filterLayout->addLayout(conditionButtons);
    filterLayout->addWidget(filterHint);

    auto *drawArea = new QWidget(this);
    auto *drawLayout = new QVBoxLayout(drawArea);
    drawLayout->setContentsMargins(0, 0, 0, 0);
    drawLayout->setSpacing(10);

    drawButton = new QPushButton("抽取", this);
    drawButton->setMinimumSize(180, 76);
    drawButton->setObjectName("drawButton");

    noRepeatCheckBox = new QCheckBox("已经抽到的人员不再抽取", this);
    noRepeatCheckBox->setChecked(false);
    noRepeatCheckBox->setToolTip(
        "开启后，已经进入历史记录的人员不会再次参与抽取");

    drawLayout->addStretch();
    drawLayout->addWidget(drawButton);
    drawLayout->addWidget(noRepeatCheckBox, 0, Qt::AlignHCenter);
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

    // UI 滚动刷新：50 ms 一次，让滚动展示更紧凑
    drawTimer = new QTimer(this);
    drawTimer->setInterval(30);

    // 后台待揭晓结果：每 1 秒重新随机一次
    drawResultTimer = new QTimer(this);
    drawResultTimer->setInterval(1000);
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
                background: #e7e9ed;
            }
            QWidget {
                background: #e7e9ed;
                color: #202124;
            }
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
            QListWidget, #personInfoWidget, #controlWidget, #historyWidget {
                background: #f8f9fb;
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
            QComboBox {
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
            #filterHint, #historyCountLabel {
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
        QListWidget, #personInfoWidget, #controlWidget, #historyWidget {
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
        QComboBox {
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
        #filterHint, #historyCountLabel {
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
        "<p>版本：1.2.0</p>"
        "<p>作者：Rogetta</p>"
        "<p>基于 Qt 6 / Qt Widgets 开发。</p>");
}
