#include "Board.h"

#include <random>

namespace core {

namespace {

// Keyed per (cell, color); fixed seed so hashes are reproducible within/across runs.
using ZobristTable = std::array<std::array<std::uint64_t, 3>, Board::kSize * Board::kSize>;

ZobristTable makeZobristTable() {
    ZobristTable table{};
    std::mt19937_64 rng(0xC0FFEE123456789ULL);
    for (auto& cell : table) {
        for (auto& key : cell) {
            key = rng();
        }
    }
    return table;
}

const ZobristTable& zobristTable() {
    static const ZobristTable table = makeZobristTable();
    return table;
}

} // namespace

Board::Board() {
    cells_.fill(Stone::Empty);
}

int Board::index(Pos p) {
    return p.row * kSize + p.col;
}

Stone Board::at(Pos p) const {
    return cells_[index(p)];
}

bool Board::inBounds(Pos p) const {
    return p.row >= 0 && p.row < kSize && p.col >= 0 && p.col < kSize;
}

bool Board::isEmpty(Pos p) const {
    return at(p) == Stone::Empty;
}

void Board::place(Pos p, Stone s) {
    const int idx = index(p);
    cells_[idx] = s;
    hash_ ^= zobristTable()[idx][static_cast<std::size_t>(s)];
    ++stoneCount_;
}

void Board::remove(Pos p) {
    const int idx = index(p);
    const Stone s = cells_[idx];
    hash_ ^= zobristTable()[idx][static_cast<std::size_t>(s)];
    cells_[idx] = Stone::Empty;
    --stoneCount_;
}

int Board::stoneCount() const {
    return stoneCount_;
}

std::uint64_t Board::hash() const {
    return hash_;
}

} // namespace core
