#pragma once

#include <optional>

#include <QWidget>

#include "Board.h"
#include "Types.h"

// 繪製棋盤並回報點擊的交叉點。只負責顯示，不知道規則或輪到誰。
class BoardView : public QWidget {
    Q_OBJECT

public:
    explicit BoardView(QWidget* parent = nullptr);

    void setBoard(const core::Board& board, std::optional<core::Pos> lastMove);
    void setInteractive(bool interactive);

    QSize sizeHint() const override;

signals:
    void cellClicked(core::Pos pos);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    core::Board m_board;
    std::optional<core::Pos> m_lastMove;
    bool m_interactive = true;
};
