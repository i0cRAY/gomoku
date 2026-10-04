#pragma once

#include <cstdint>

namespace core {

enum class Stone : std::uint8_t { Empty, Black, White };

inline Stone opponent(Stone s) {
    switch (s) {
        case Stone::Black:
            return Stone::White;
        case Stone::White:
            return Stone::Black;
        default:
            return Stone::Empty;
    }
}

struct Pos {
    int row;
    int col;
    bool operator==(const Pos&) const = default;
};

struct Move {
    Pos pos;
    Stone color;
    std::int64_t timeUsedMs;
};

enum class GameResult { Ongoing, BlackWin, WhiteWin, Draw };
enum class ResultReason { None, FiveInRow, Timeout, BoardFull, Resign, Disconnect };

} // namespace core
