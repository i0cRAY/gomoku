#include "GameState.h"

#include "Rules.h"

namespace core {

bool GameState::play(Pos p, std::int64_t timeUsedMs) {
    if (result_ != GameResult::Ongoing) {
        return false;
    }
    if (!Rules::isLegal(board_, p)) {
        return false;
    }

    const Stone color = sideToMove_;
    board_.place(p, color);
    history_.push(Move{p, color, timeUsedMs});

    if (Rules::makesFive(board_, p)) {
        result_ = color == Stone::Black ? GameResult::BlackWin : GameResult::WhiteWin;
        reason_ = ResultReason::FiveInRow;
    } else if (Rules::isFull(board_)) {
        result_ = GameResult::Draw;
        reason_ = ResultReason::BoardFull;
    } else {
        sideToMove_ = opponent(color);
    }
    return true;
}

bool GameState::undo(int plies) {
    if (result_ != GameResult::Ongoing) {
        return false;
    }
    if (plies <= 0 || static_cast<std::size_t>(plies) > history_.size()) {
        return false;
    }

    lastUndone_.clear();
    for (int i = 0; i < plies; ++i) {
        const auto m = history_.pop();
        board_.remove(m->pos);
        lastUndone_.push_back(*m);
    }
    sideToMove_ = lastUndone_.back().color;
    return true;
}

void GameState::finish(GameResult r, ResultReason why) {
    result_ = r;
    reason_ = why;
}

const Board& GameState::board() const {
    return board_;
}

const MoveHistory& GameState::history() const {
    return history_;
}

Stone GameState::sideToMove() const {
    return sideToMove_;
}

GameResult GameState::result() const {
    return result_;
}

ResultReason GameState::reason() const {
    return reason_;
}

const std::vector<Move>& GameState::lastUndone() const {
    return lastUndone_;
}

} // namespace core
