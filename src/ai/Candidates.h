#pragma once

#include <vector>

#include "Board.h"
#include "Types.h"

namespace ai {

// 候選步（SDD §5.2.1）：已有棋子周圍 radius 格內（8 方向）的空點，空盤時只有天元。
// 依「進攻 + 防守」增益由高到低排序，同分時 row 小的優先、再比 col；最多 maxCount 個。
std::vector<core::Pos> generateCandidates(const core::Board& b, core::Stone side, int radius, int maxCount);

} // namespace ai
