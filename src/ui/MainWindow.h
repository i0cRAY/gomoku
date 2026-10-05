#pragma once

#include <QMainWindow>

#include "GameState.h"

class BoardView;
class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void onCellClicked(core::Pos pos);
    void refresh();

    core::GameState m_state;
    BoardView* m_boardView = nullptr;
    QLabel* m_statusLabel = nullptr;
};
