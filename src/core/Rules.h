#pragma once

#include "Board.h"

namespace core::Rules {

bool isLegal(const Board& b, Pos p);
bool makesFive(const Board& b, Pos lastMove);
bool isFull(const Board& b);

} // namespace core::Rules
