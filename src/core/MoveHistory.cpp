#include "MoveHistory.h"

namespace core {

void MoveHistory::push(const Move& m) {
    m_moves.push_back(m);
}

std::optional<Move> MoveHistory::pop() {
    if (m_moves.empty()) {
        return std::nullopt;
    }
    const Move m = m_moves.back();
    m_moves.pop_back();
    return m;
}

const std::vector<Move>& MoveHistory::moves() const {
    return m_moves;
}

std::size_t MoveHistory::size() const {
    return m_moves.size();
}

} // namespace core
