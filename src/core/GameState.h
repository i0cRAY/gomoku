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
    Board board_;
    MoveHistory history_;
    Stone sideToMove_ = Stone::Black;
    GameResult result_ = GameResult::Ongoing;
    ResultReason reason_ = ResultReason::None;
    std::vector<Move> lastUndone_;
};

} // namespace core
