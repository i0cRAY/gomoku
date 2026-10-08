#pragma once

#include <vector>

#include "Board.h"
#include "Types.h"

namespace ai::Evaluator {

// 棋型（SDD §5.2.2）；由弱到強排列
enum class Pattern { None, Two, OpenTwo, Three, OpenThree, Four, OpenFour, Five };

int patternScore(Pattern p);

// 局面分，以 side 的角度：己方分數 − 對方分數 × 11 / 10
int evaluate(const core::Board& b, core::Stone side);

// side 所有棋型分數的總和（不扣對方）
int sideScore(const core::Board& b, core::Stone side);

// side 下在空點 p 時，通過 p 的四條線上 side 棋型分數的增加量（候選步排序用，不修改棋盤）
int pointGain(const core::Board& b, core::Pos p, core::Stone side);

// 一條線上 side 各棋組的棋型，依線上位置由左到右，不含 None。線的兩端視為棋盤邊界
std::vector<Pattern> patternsInLine(const std::vector<core::Stone>& line, core::Stone side);

} // namespace ai::Evaluator
