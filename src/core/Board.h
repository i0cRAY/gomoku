#pragma once

#include <array>
#include <cstdint>

#include "Types.h"

namespace core {

class Board {
public:
    static constexpr int kSize = 15;

    Board();

    Stone at(Pos p) const;
    bool inBounds(Pos p) const;
    bool isEmpty(Pos p) const;
    void place(Pos p, Stone s);
    void remove(Pos p);
    int stoneCount() const;
    std::uint64_t hash() const;

private:
    static int index(Pos p);

    std::array<Stone, kSize * kSize> cells_;
    int stoneCount_ = 0;
    std::uint64_t hash_ = 0;
};

} // namespace core
