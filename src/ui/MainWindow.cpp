#include "MainWindow.h"

#include <optional>

#include <QLabel>
#include <QMessageBox>
#include <QStatusBar>

#include "BoardView.h"

namespace {

QString sideName(core::Stone s) {
    return s == core::Stone::Black ? MainWindow::tr("黑方") : MainWindow::tr("白方");
}

QString resultText(core::GameResult r, core::ResultReason why) {
    QString winner;
    switch (r) {
        case core::GameResult::BlackWin:
            winner = MainWindow::tr("黑方勝");
            break;
        case core::GameResult::WhiteWin:
            winner = MainWindow::tr("白方勝");
            break;
        case core::GameResult::Draw:
            winner = MainWindow::tr("和局");
            break;
        case core::GameResult::Ongoing:
            return {};
    }
    switch (why) {
        case core::ResultReason::FiveInRow:
            return MainWindow::tr("%1（連五）").arg(winner);
        case core::ResultReason::BoardFull:
            return MainWindow::tr("%1（棋盤已滿）").arg(winner);
        default:
            return winner;
    }
}

} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    resize(640, 680);
    setWindowTitle(tr("Gomoku"));

    m_boardView = new BoardView(this);
    setCentralWidget(m_boardView);
    connect(m_boardView, &BoardView::cellClicked, this, &MainWindow::onCellClicked);

    m_statusLabel = new QLabel(this);
    statusBar()->addWidget(m_statusLabel);

    refresh();
}

void MainWindow::onCellClicked(core::Pos pos) {
    if (!m_state.play(pos, 0)) {
        return;
    }
    refresh();
    if (m_state.result() != core::GameResult::Ongoing) {
        QMessageBox::information(this, tr("對局結束"), resultText(m_state.result(), m_state.reason()));
    }
}

void MainWindow::refresh() {
    const auto& moves = m_state.history().moves();
    const std::optional<core::Pos> lastMove =
        moves.empty() ? std::nullopt : std::optional<core::Pos>(moves.back().pos);
    m_boardView->setBoard(m_state.board(), lastMove);

    const bool ongoing = m_state.result() == core::GameResult::Ongoing;
    m_boardView->setInteractive(ongoing);
    m_statusLabel->setText(ongoing ? tr("輪到%1").arg(sideName(m_state.sideToMove()))
                                   : resultText(m_state.result(), m_state.reason()));
}
