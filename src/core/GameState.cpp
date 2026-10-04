#include "GameState.h"

#include "Rules.h"

namespace core {

bool GameState::play(Pos p, std::int64_t timeUsedMs) {
    if (m_result != GameResult::Ongoing) {
        return false;
    }
    if (!Rules::isLegal(m_board, p)) {
        return false;
    }

    const Stone color = m_sideToMove;
    m_board.place(p, color);
    m_history.push(Move{p, color, timeUsedMs});

    if (Rules::makesFive(m_board, p)) {
        m_result = color == Stone::Black ? GameResult::BlackWin : GameResult::WhiteWin;
        m_reason = ResultReason::FiveInRow;
    } else if (Rules::isFull(m_board)) {
        m_result = GameResult::Draw;
        m_reason = ResultReason::BoardFull;
    } else {
        m_sideToMove = opponent(color);
    }
    return true;
}

bool GameState::undo(int plies) {
    // Cleared up front so a failed undo never exposes a previous undo's moves.
    m_lastUndone.clear();
    if (m_result != GameResult::Ongoing) {
        return false;
    }
    if (plies <= 0 || static_cast<std::size_t>(plies) > m_history.size()) {
        return false;
    }

    for (int i = 0; i < plies; ++i) {
        const auto m = m_history.pop();
        m_board.remove(m->pos);
        m_lastUndone.push_back(*m);
    }
    m_sideToMove = m_lastUndone.back().color;
    return true;
}

void GameState::finish(GameResult r, ResultReason why) {
    // First result wins: a late timeout/disconnect must not overwrite it.
    // finish() only ends a game; it can never set it back to Ongoing.
    if (m_result != GameResult::Ongoing || r == GameResult::Ongoing) {
        return;
    }
    m_result = r;
    m_reason = why;
}

const Board& GameState::board() const {
    return m_board;
}

const MoveHistory& GameState::history() const {
    return m_history;
}

Stone GameState::sideToMove() const {
    return m_sideToMove;
}

GameResult GameState::result() const {
    return m_result;
}

ResultReason GameState::reason() const {
    return m_reason;
}

const std::vector<Move>& GameState::lastUndone() const {
    return m_lastUndone;
}

} // namespace core
