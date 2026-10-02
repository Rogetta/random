// =============================================================================
// MainWindowDraw.cpp —— 抽取与历史板块
// 负责随机抽取动画的启停、滚动展示、每秒更新待揭晓结果以及历史记录。
// =============================================================================

#include "MainWindow.h"

#include <QCheckBox>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSet>
#include <QTimer>

#include <cstdint>

std::uint32_t get_random_bounded(std::uint32_t upperBound);

// =============================================================================
// 开始 / 停止抽取
// =============================================================================

void MainWindow::toggleDraw()
{
    // -------------------------------------------------------------------------
    // 正在滚动 -> 用户按下“停止”
    //
    // 注意：
    // currentDrawIndex 是 UI 当前滚动显示的人；
    // pendingDrawIndex 才是后台每秒确定的最终结果。
    // -------------------------------------------------------------------------
    if (drawing) {
        drawing = false;

        drawTimer->stop();
        drawResultTimer->stop();

        drawButton->setText("抽取");

        // 用户停止后，揭晓最近一次后台随机确定的结果
        if (pendingDrawIndex >= 0 &&
            currentListIndex >= 0 &&
            currentListIndex < nameLists.size() &&
            pendingDrawIndex <
                nameLists[currentListIndex].people.size()) {

            currentDrawIndex = pendingDrawIndex;

            updatePersonInformation(
                nameLists[currentListIndex]
                    .people[currentDrawIndex]);

            historyIndices.append(currentDrawIndex);

            auto *historyItem = new QListWidgetItem(
                nameLists[currentListIndex]
                    .people[currentDrawIndex].name,
                historyListWidget);

            historyItem->setData(
                Qt::UserRole,
                currentDrawIndex);

            historyListWidget->scrollToBottom();

            historyCountLabel->setText(
                QString("共%1条记录")
                    .arg(historyIndices.size()));

            // 开启“不重复”时，刚刚抽中的人员立即从候选池移除
            filterChanged();
        }

        pendingDrawIndex = -1;
        return;
    }

    // -------------------------------------------------------------------------
    // 开始抽取
    // -------------------------------------------------------------------------

    // 每次开始前重新计算候选集合。
    // 后续所有随机结果都只能来自这里。
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

    // 抽取开始时先确定一个待揭晓结果。
    // 此结果不会因为 50 ms 的 UI 滚动而改变。
    updateDrawResult();

    // UI 高速滚动
    drawTimer->start();

    // 每秒重新确定一次最终结果
    drawResultTimer->start();
}


// =============================================================================
// UI 滚动动画
//
// 这里的随机结果只是视觉上的“滚动”，不参与最终抽取。
// =============================================================================

void MainWindow::drawStep()
{
    if (!drawing ||
        drawCandidates.isEmpty() ||
        currentListIndex < 0 ||
        currentListIndex >= nameLists.size()) {
        return;
    }

    const auto randomIndex =
        get_random_bounded(
            static_cast<std::uint32_t>(
                drawCandidates.size()));

    const int displayIndex =
        drawCandidates.at(
            static_cast<int>(randomIndex));

    // 只更新 UI 当前展示，不覆盖后台待揭晓结果
    currentDrawIndex = displayIndex;

    updatePersonInformation(
        nameLists[currentListIndex]
            .people[displayIndex]);
}


// =============================================================================
// 每 1 秒重新随机一次“最终结果”
//
// 用户可以在任意时间按下停止。
// 停止时使用最近一次 pendingDrawIndex。
// =============================================================================

void MainWindow::updateDrawResult()
{
    if (!drawing ||
        drawCandidates.isEmpty() ||
        currentListIndex < 0 ||
        currentListIndex >= nameLists.size()) {
        return;
    }

    const auto randomIndex =
        get_random_bounded(
            static_cast<std::uint32_t>(
                drawCandidates.size()));

    pendingDrawIndex =
        drawCandidates.at(
            static_cast<int>(randomIndex));
}


// =============================================================================
// 清除历史
// =============================================================================

void MainWindow::clearHistory()
{
    if (drawing)
        toggleDraw();

    historyIndices.clear();
    historyListWidget->clear();
    historyCountLabel->setText("共0条记录");
    currentDrawIndex = -1;
    pendingDrawIndex = -1;

    if (currentListIndex >= 0 &&
        currentListIndex < nameLists.size())
        filterChanged();
}


// =============================================================================
// 不重复抽取
// =============================================================================

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
        "autoLoadLastList",
        checked);
}
