#pragma once

#include <QMainWindow>
#include <QString>

#include "GameRecord.h"
#include "GameState.h"

class BoardView;
class QAction;
class QLabel;

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
    void onCellClicked(core::Pos pos);
    void refresh();

    core::GameState m_state;
    BoardView* m_boardView = nullptr;
    QLabel* m_statusLabel = nullptr;
    QAction* m_exportAction = nullptr;
    QString m_lastDir;   // 上次存讀檔的資料夾
};
