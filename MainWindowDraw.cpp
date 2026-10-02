// =============================================================================
// MainWindowDraw.cpp —— 抽取与历史板块
// 负责随机抽取动画的启停（toggleDraw / drawStep）、历史记录的维护
// （clearHistory）以及“不重复抽取”开关（setNoRepeat）等设置项。
// =============================================================================

#include "MainWindow.h"

#include <QCheckBox>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSettings>
#include <QSet>
#include <QTimer>

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

            historyIndices.append(currentDrawIndex);

            auto *historyItem = new QListWidgetItem(
                nameLists[currentListIndex]
                    .people[currentDrawIndex].name,
                historyListWidget);
            historyItem->setData(
                Qt::UserRole, currentDrawIndex);

            historyListWidget->scrollToBottom();
            historyCountLabel->setText(
                QString("共%1条记录")
                    .arg(historyIndices.size()));

            filterChanged();
        }

        return;
    }

    filterChanged();

    if (drawCandidates.isEmpty()) {
        QSet<int> uniqueHistory;
        for (const int index : historyIndices)
            uniqueHistory.insert(index);

        if (noRepeatCheckBox->isChecked() &&
            !nameLists[currentListIndex].people.isEmpty() &&
            uniqueHistory.size() >=
                nameLists[currentListIndex].people.size()) {
            QMessageBox::information(
                this,
                "抽取完成",
                "所有人都已被抽过，请清除记录");
        } else {
            QMessageBox::information(
                this,
                "无法抽取",
                "当前没有符合筛选条件的人员。");
        }
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

void MainWindow::clearHistory()
{
    if (drawing)
        toggleDraw();

    historyIndices.clear();
    historyListWidget->clear();
    historyCountLabel->setText("共0条记录");
    currentDrawIndex = -1;

    if (currentListIndex >= 0 &&
        currentListIndex < nameLists.size())
        filterChanged();
}

void MainWindow::setNoRepeat(bool checked)
{
    noRepeatAction->blockSignals(true);
    noRepeatAction->setChecked(checked);
    noRepeatAction->blockSignals(false);

    noRepeatCheckBox->blockSignals(true);
    noRepeatCheckBox->setChecked(checked);
    noRepeatCheckBox->blockSignals(false);

    QSettings().setValue("noRepeatDraw", checked);
    filterChanged();
}

void MainWindow::autoLoadLastListChanged(bool checked)
{
    QSettings().setValue(
        "autoLoadLastList", checked);
}
