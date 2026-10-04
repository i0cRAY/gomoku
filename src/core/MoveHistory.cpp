#include "MoveHistory.h"

namespace core {

void MoveHistory::push(const Move& m) {
    moves_.push_back(m);
}

std::optional<Move> MoveHistory::pop() {
    if (moves_.empty()) {
        return std::nullopt;
    }
    const Move m = moves_.back();
    moves_.pop_back();
    return m;
}

const std::vector<Move>& MoveHistory::moves() const {
    return moves_;
}

std::size_t MoveHistory::size() const {
    return moves_.size();
}

} // namespace core
