#pragma once

#include <optional>

#include <QMainWindow>
#include <QString>

#include "GameRecord.h"
#include "GameState.h"
#include "ReplayCursor.h"

class BoardView;
class QAction;
class QLabel;
class QTimer;
class ReplayControls;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void newGame();
    void saveGame();
    void loadGame();
    void exportRecord();
    void writeRecordFile(const QString& title);
    core::GameRecord currentRecord() const;
    void startReplay();
    void exitReplay();
    void replayStep(bool (core::ReplayCursor::*move)());
    void toggleAutoPlay();
    void onAutoPlayTick();
    void stopAutoPlay();
    void onCellClicked(core::Pos pos);
    void refresh();

    core::GameState m_state;
    BoardView* m_boardView = nullptr;
    QLabel* m_statusLabel = nullptr;
    QAction* m_saveAction = nullptr;
    QAction* m_exportAction = nullptr;
    ReplayControls* m_replayControls = nullptr;
    QTimer* m_autoPlayTimer = nullptr;

    // 回放模式：有值時棋盤顯示回放內容，m_state 不使用
    std::optional<core::ReplayCursor> m_replay;
    core::GameResult m_replayResult = core::GameResult::Ongoing;
    core::ResultReason m_replayReason = core::ResultReason::None;
    QString m_lastDir;   // 上次存讀檔的資料夾
};
