#pragma once

#include <vector>

#include "Board.h"
#include "MoveHistory.h"
#include "Types.h"

namespace core {

class GameState {
public:
    bool play(Pos p, std::int64_t timeUsedMs);
    bool undo(int plies);
    void finish(GameResult r, ResultReason why);

    const Board& board() const;
    const MoveHistory& history() const;
    Stone sideToMove() const;
    GameResult result() const;
    ResultReason reason() const;
    const std::vector<Move>& lastUndone() const;

private:
    Board m_board;
    MoveHistory m_history;
    Stone m_sideToMove = Stone::Black;
    GameResult m_result = GameResult::Ongoing;
    ResultReason m_reason = ResultReason::None;
    std::vector<Move> m_lastUndone;
};

} // namespace core
