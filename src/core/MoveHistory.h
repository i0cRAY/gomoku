#pragma once

#include <optional>
#include <vector>

#include "Types.h"

namespace core {

class MoveHistory {
public:
    void push(const Move& m);
    std::optional<Move> pop();
    const std::vector<Move>& moves() const;
    std::size_t size() const;

private:
    std::vector<Move> moves_;
};

} // namespace core
