#include "Rules.h"

namespace core::Rules {

namespace {

constexpr Pos kDirections[4] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

int countDirection(const Board& b, Pos from, Pos dir, Stone s) {
    int count = 0;
    Pos p{from.row + dir.row, from.col + dir.col};
    while (b.inBounds(p) && b.at(p) == s) {
        ++count;
        p.row += dir.row;
        p.col += dir.col;
    }
    return count;
}

} // namespace

bool isLegal(const Board& b, Pos p) {
    return b.inBounds(p) && b.isEmpty(p);
}

bool makesFive(const Board& b, Pos lastMove) {
    const Stone s = b.at(lastMove);
    if (s == Stone::Empty) {
        return false;
    }
    for (const Pos dir : kDirections) {
        const int forward = countDirection(b, lastMove, dir, s);
        const Pos back{-dir.row, -dir.col};
        const int backward = countDirection(b, lastMove, back, s);
        if (1 + forward + backward >= 5) {
            return true;
        }
    }
    return false;
}

bool isFull(const Board& b) {
    return b.stoneCount() == Board::kSize * Board::kSize;
}

} // namespace core::Rules
