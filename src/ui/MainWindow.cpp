#include "MainWindow.h"

#include <optional>
#include <variant>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStatusBar>

#include "BoardView.h"
#include "SaveFormat.h"

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

const QString kFileSuffix = QStringLiteral(".gomoku.json");
constexpr int kStatusMessageMs = 3000;

QString fileFilter() {
    return MainWindow::tr("五子棋存檔 (*.gomoku.json)");
}

// 含時區的 ISO 8601 時間，例如 2026-10-05T10:00:00+08:00
QString nowIso8601() {
    const QDateTime now = QDateTime::currentDateTime();
    return now.toOffsetFromUtc(now.offsetFromUtc()).toString(Qt::ISODate);
}

struct LoadedGame {
    core::GameRecord record;
    core::GameState state;
};

// 讀檔並完整驗證（格式 + 重播），失敗回傳給使用者看的錯誤訊息
std::variant<LoadedGame, QString> readRecordFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return MainWindow::tr("無法開啟檔案：%1").arg(file.errorString());
    }
    auto parsed = SaveFormat::fromJson(file.readAll());
    if (const auto* err = std::get_if<QString>(&parsed)) {
        return *err;
    }
    core::GameRecord record = std::get<core::GameRecord>(std::move(parsed));
    auto restored = core::restore(record);
    if (const auto* err = std::get_if<std::string>(&restored)) {
        return QString::fromStdString(*err);
    }
    return LoadedGame{std::move(record), std::get<core::GameState>(std::move(restored))};
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
    gameMenu->addAction(tr("存檔(&S)…"), QKeySequence::Save, this, &MainWindow::saveGame);
    gameMenu->addAction(tr("讀檔(&O)…"), QKeySequence::Open, this, &MainWindow::loadGame);
    m_exportAction = gameMenu->addAction(tr("匯出棋譜(&E)…"), this, &MainWindow::exportRecord);
    gameMenu->addSeparator();
    gameMenu->addAction(tr("離開(&Q)"), QKeySequence(Qt::CTRL | Qt::Key_Q), this, &QWidget::close);

    m_statusLabel = new QLabel(this);
    statusBar()->addWidget(m_statusLabel);

    m_lastDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    refresh();
}

void MainWindow::newGame() {
    m_state = core::GameState{};
    refresh();
}

void MainWindow::saveGame() {
    writeRecordFile(tr("存檔"));
}

// 棋譜與存檔格式相同（SDD §6.1），差別只在對局結束後才能匯出
void MainWindow::exportRecord() {
    if (m_state.result() == core::GameResult::Ongoing) {
        return;
    }
    writeRecordFile(tr("匯出棋譜"));
}

void MainWindow::writeRecordFile(const QString& title) {
    const QString defaultName = QDateTime::currentDateTime().toString("'gomoku-'yyyyMMdd-HHmmss") + kFileSuffix;
    QString path = QFileDialog::getSaveFileName(this, title, QDir(m_lastDir).filePath(defaultName), fileFilter());
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(kFileSuffix)) {
        // 補上副檔名後是另一個檔案，對話框沒有幫忙確認過覆蓋
        path += kFileSuffix;
        if (QFileInfo::exists(path) &&
            QMessageBox::question(this, title, tr("%1 已存在，要覆蓋嗎？").arg(QFileInfo(path).fileName())) !=
                QMessageBox::Yes) {
            return;
        }
    }
    m_lastDir = QFileInfo(path).absolutePath();

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(SaveFormat::toJson(currentRecord())) < 0 ||
        !file.commit()) {
        QMessageBox::warning(this, title, tr("無法寫入檔案：%1").arg(file.errorString()));
        return;
    }
    statusBar()->showMessage(tr("已儲存到 %1").arg(QFileInfo(path).fileName()), kStatusMessageMs);
}

void MainWindow::loadGame() {
    const QString path = QFileDialog::getOpenFileName(this, tr("讀檔"), m_lastDir, fileFilter());
    if (path.isEmpty()) {
        return;
    }
    m_lastDir = QFileInfo(path).absolutePath();

    auto loaded = readRecordFile(path);
    if (const auto* err = std::get_if<QString>(&loaded)) {
        QMessageBox::warning(this, tr("讀檔失敗"), *err);
        return;
    }
    auto& game = std::get<LoadedGame>(loaded);
    // 目前只實作本機雙人、不限時（SDD §6.1 讀檔繼續的支援範圍）
    if (game.record.matchType != core::MatchType::Local || game.record.timeControl.timed) {
        QMessageBox::warning(this, tr("讀檔失敗"), tr("目前只支援讀取本機雙人、不限時的對局"));
        return;
    }
    m_state = std::move(game.state);
    refresh();
    statusBar()->showMessage(tr("已讀取 %1").arg(QFileInfo(path).fileName()), kStatusMessageMs);
}

core::GameRecord MainWindow::currentRecord() const {
    core::GameRecord r;   // 預設即為本機雙人、不限時
    r.createdAt = nowIso8601().toStdString();
    r.moves = m_state.history().moves();
    r.result = m_state.result();
    r.reason = m_state.reason();
    return r;
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
    m_exportAction->setEnabled(!ongoing);
    m_statusLabel->setText(ongoing ? tr("輪到%1").arg(sideName(m_state.sideToMove()))
                                   : resultText(m_state.result(), m_state.reason()));
}
