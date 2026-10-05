#include "ReplayCursor.h"

#include <utility>

namespace core {

ReplayCursor::ReplayCursor(std::vector<Move> moves) : m_moves(std::move(moves)) {}

std::size_t ReplayCursor::index() const {
    return m_index;
}

std::size_t ReplayCursor::size() const {
    return m_moves.size();
}

const Board& ReplayCursor::board() const {
    return m_board;
}

std::optional<Pos> ReplayCursor::lastMove() const {
    if (m_index == 0) {
        return std::nullopt;
    }
    return m_moves[m_index - 1].pos;
}

bool ReplayCursor::toStart() {
    if (m_index == 0) {
        return false;
    }
    while (prev()) {
    }
    return true;
}

bool ReplayCursor::prev() {
    if (m_index == 0) {
        return false;
    }
    --m_index;
    m_board.remove(m_moves[m_index].pos);
    return true;
}

bool ReplayCursor::next() {
    if (m_index == m_moves.size()) {
        return false;
    }
    const Move& m = m_moves[m_index];
    m_board.place(m.pos, m.color);
    ++m_index;
    return true;
}

bool ReplayCursor::toEnd() {
    if (m_index == m_moves.size()) {
        return false;
    }
    while (next()) {
    }
    return true;
}

} // namespace core
