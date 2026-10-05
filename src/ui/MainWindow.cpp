#include "MainWindow.h"

#include <optional>

#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
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
    resize(640, 700);
    setWindowTitle(tr("Gomoku"));

    m_boardView = new BoardView(this);
    setCentralWidget(m_boardView);
    connect(m_boardView, &BoardView::cellClicked, this, &MainWindow::onCellClicked);

    // 固定在視窗內顯示，不交給 KDE 等桌面的全域選單
    menuBar()->setNativeMenuBar(false);
    QMenu* gameMenu = menuBar()->addMenu(tr("遊戲(&G)"));
    gameMenu->addAction(tr("新對局(&N)"), QKeySequence::New, this, &MainWindow::newGame);
    gameMenu->addSeparator();
    gameMenu->addAction(tr("離開(&Q)"), QKeySequence(Qt::CTRL | Qt::Key_Q), this, &QWidget::close);

    m_statusLabel = new QLabel(this);
    statusBar()->addWidget(m_statusLabel);

    refresh();
}

void MainWindow::newGame() {
    m_state = core::GameState{};
    refresh();
}

void MainWindow::onCellClicked(core::Pos pos) {
    if (!m_state.play(pos, 0)) {
        return;
    }
    refresh();
    if (m_state.result() != core::GameResult::Ongoing) {
        QMessageBox box(QMessageBox::Information, tr("對局結束"), resultText(m_state.result(), m_state.reason()),
                        QMessageBox::NoButton, this);
        QPushButton* again = box.addButton(tr("再來一局"), QMessageBox::AcceptRole);
        box.addButton(tr("關閉"), QMessageBox::RejectRole);
        box.setDefaultButton(again);
        box.exec();
        if (box.clickedButton() == again) {
            newGame();
        }
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
