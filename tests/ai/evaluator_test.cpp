#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "Board.h"
#include "Evaluator.h"

using namespace core;
using ai::Evaluator::Pattern;

namespace {

// 以字串描述一條線：X = 己方（黑），O = 對方（白），_ = 空；字串兩端即棋盤邊界
std::vector<Pattern> patterns(const std::string& text) {
    std::vector<Stone> line;
    for (const char ch : text) {
        line.push_back(ch == 'X' ? Stone::Black : ch == 'O' ? Stone::White : Stone::Empty);
    }
    return ai::Evaluator::patternsInLine(line, Stone::Black);
}

using P = std::vector<Pattern>;

void placeAll(Board& b, const std::vector<Pos>& cells, Stone s) {
    for (const Pos p : cells) {
        b.place(p, s);
    }
}

} // namespace

// ---- 單一條線的棋型判定（SDD §5.2.2 表格） ----

TEST(Evaluator, Five) {
    EXPECT_EQ(patterns("XXXXX"), (P{Pattern::Five}));
    EXPECT_EQ(patterns("_XXXXXX_"), (P{Pattern::Five}));   // 六連也算
    EXPECT_EQ(patterns("OXXXXXO"), (P{Pattern::Five}));    // 已成五，被擋也算
}

TEST(Evaluator, OpenFour) {
    EXPECT_EQ(patterns("_XXXX_"), (P{Pattern::OpenFour}));
    EXPECT_EQ(patterns("X_XXX_X"), (P{Pattern::OpenFour}));   // 兩個點都能成五
    EXPECT_EQ(patterns("O_XXXX_O"), (P{Pattern::OpenFour}));
}

TEST(Evaluator, Four) {
    EXPECT_EQ(patterns("OXXXX_"), (P{Pattern::Four}));
    EXPECT_EQ(patterns("XXXX__"), (P{Pattern::Four}));   // 左邊是棋盤邊界
    EXPECT_EQ(patterns("X_XXX"), (P{Pattern::Four}));
    EXPECT_EQ(patterns("XX_XX"), (P{Pattern::Four}));
    EXPECT_EQ(patterns("_XXX_X_"), (P{Pattern::Four}));
}

TEST(Evaluator, FourNegative) {
    EXPECT_EQ(patterns("OXXXXO"), P{});   // 區段只有 4 格，不可能成五
    EXPECT_EQ(patterns("_XXXX_"), (P{Pattern::OpenFour}));
}

TEST(Evaluator, OpenThree) {
    EXPECT_EQ(patterns("__XXX_"), (P{Pattern::OpenThree}));
    EXPECT_EQ(patterns("_XXX__"), (P{Pattern::OpenThree}));
    EXPECT_EQ(patterns("_XX_X_"), (P{Pattern::OpenThree}));
    EXPECT_EQ(patterns("_X_XX_"), (P{Pattern::OpenThree}));
}

TEST(Evaluator, OpenThreeNegative) {
    EXPECT_EQ(patterns("O_XXX_O"), (P{Pattern::Three}));   // 兩邊各只有一格，做不出活四
    EXPECT_EQ(patterns("OXXX___"), (P{Pattern::Three}));   // 一端被擋
}

TEST(Evaluator, Three) {
    EXPECT_EQ(patterns("OXXX__"), (P{Pattern::Three}));
    EXPECT_EQ(patterns("X_X_X"), (P{Pattern::Three}));
    EXPECT_EQ(patterns("O_XXX_O"), (P{Pattern::Three}));
    EXPECT_EQ(patterns("XX_X_"), (P{Pattern::Three}));
}

TEST(Evaluator, ThreeNegative) {
    EXPECT_EQ(patterns("OXXX_O"), P{});                    // 區段只有 4 格
    EXPECT_EQ(patterns("__XXX__"), (P{Pattern::OpenThree}));
}

TEST(Evaluator, OpenTwo) {
    EXPECT_EQ(patterns("__XX__"), (P{Pattern::OpenTwo}));
    EXPECT_EQ(patterns("_X_X__"), (P{Pattern::OpenTwo}));
    EXPECT_EQ(patterns("___XX___"), (P{Pattern::OpenTwo}));
}

TEST(Evaluator, OpenTwoNegative) {
    EXPECT_EQ(patterns("OXX___"), (P{Pattern::Two}));      // 一端被擋
    EXPECT_EQ(patterns("O_XX_O"), P{});                    // 區段只有 4 格
}

TEST(Evaluator, Two) {
    EXPECT_EQ(patterns("OXX___"), (P{Pattern::Two}));
    EXPECT_EQ(patterns("OX_X__"), (P{Pattern::Two}));
    EXPECT_EQ(patterns("XX___"), (P{Pattern::Two}));       // 左邊是棋盤邊界
}

TEST(Evaluator, TwoNegative) {
    EXPECT_EQ(patterns("OXXO"), P{});
    EXPECT_EQ(patterns("X___X"), P{});     // 間隔超過 1 格，是兩個單子
}

TEST(Evaluator, SingleStoneHasNoPattern) {
    EXPECT_EQ(patterns("__X__"), P{});
    EXPECT_EQ(patterns("_______"), P{});
}

TEST(Evaluator, GroupsInOneLineAreClassifiedSeparately) {
    EXPECT_EQ(patterns("__XX___XXX__"), (P{Pattern::OpenTwo, Pattern::OpenThree}));
}

TEST(Evaluator, OpponentStonesSplitSegments) {
    EXPECT_EQ(patterns("_XXXX_OXXX__"), (P{Pattern::OpenFour, Pattern::Three}));
    EXPECT_EQ(patterns("XXOXXOXX"), P{});   // 每段都太短
}

TEST(Evaluator, LineShorterThanFiveHasNoPattern) {
    EXPECT_EQ(patterns("XXXX"), P{});
}

TEST(Evaluator, PatternsAreFromGivenSidesPointOfView) {
    std::vector<Stone> line(9, Stone::Empty);
    for (int i = 2; i <= 4; ++i) {
        line[i] = Stone::White;
    }
    EXPECT_EQ(ai::Evaluator::patternsInLine(line, Stone::White), (P{Pattern::OpenThree}));
    EXPECT_EQ(ai::Evaluator::patternsInLine(line, Stone::Black), P{});
}

// ---- 分數 ----

TEST(Evaluator, PatternScoresMatchSdd) {
    EXPECT_EQ(ai::Evaluator::patternScore(Pattern::Five), 10'000'000);
    EXPECT_EQ(ai::Evaluator::patternScore(Pattern::OpenFour), 100'000);
    EXPECT_EQ(ai::Evaluator::patternScore(Pattern::Four), 10'000);
    EXPECT_EQ(ai::Evaluator::patternScore(Pattern::OpenThree), 5'000);
    EXPECT_EQ(ai::Evaluator::patternScore(Pattern::Three), 500);
    EXPECT_EQ(ai::Evaluator::patternScore(Pattern::OpenTwo), 200);
    EXPECT_EQ(ai::Evaluator::patternScore(Pattern::Two), 50);
    EXPECT_EQ(ai::Evaluator::patternScore(Pattern::None), 0);
}

TEST(Evaluator, EmptyBoardIsZero) {
    Board b;
    EXPECT_EQ(ai::Evaluator::evaluate(b, Stone::Black), 0);
    EXPECT_EQ(ai::Evaluator::evaluate(b, Stone::White), 0);
}

TEST(Evaluator, OpenThreeScoredFromBothSides) {
    Board b;
    placeAll(b, {{7, 6}, {7, 7}, {7, 8}}, Stone::Black);
    // 橫向活三 5000；其他方向都是單子
    EXPECT_EQ(ai::Evaluator::evaluate(b, Stone::Black), 5000);
    EXPECT_EQ(ai::Evaluator::evaluate(b, Stone::White), -5500);   // 對方分數 × 11 / 10
}

TEST(Evaluator, AllFourDirectionsCount) {
    const std::vector<std::vector<Pos>> threes{
        {{6, 7}, {7, 7}, {8, 7}},   // 直
        {{6, 6}, {7, 7}, {8, 8}},   // 左上到右下
        {{6, 8}, {7, 7}, {8, 6}},   // 右上到左下
    };
    for (const auto& cells : threes) {
        Board b;
        placeAll(b, cells, Stone::Black);
        EXPECT_EQ(ai::Evaluator::evaluate(b, Stone::Black), 5000);
    }
}

TEST(Evaluator, BlockedThreeScoresLess) {
    Board b;
    placeAll(b, {{7, 6}, {7, 7}, {7, 8}}, Stone::Black);
    b.place({7, 5}, Stone::White);
    EXPECT_EQ(ai::Evaluator::evaluate(b, Stone::Black), 500);   // 變成眠三；白方單子 0 分
}

TEST(Evaluator, DefenseWeighted) {
    Board b;
    placeAll(b, {{2, 6}, {2, 7}, {2, 8}}, Stone::Black);
    placeAll(b, {{12, 6}, {12, 7}, {12, 8}}, Stone::White);
    EXPECT_EQ(ai::Evaluator::evaluate(b, Stone::Black), 5000 - 5500);
    EXPECT_EQ(ai::Evaluator::evaluate(b, Stone::White), 5000 - 5500);
}

TEST(Evaluator, SwappingColorsSwapsPerspective) {
    Board b;
    Board swapped;
    const std::vector<Pos> black{{7, 7}, {7, 8}, {8, 8}, {6, 9}, {3, 3}};
    const std::vector<Pos> white{{7, 9}, {8, 7}, {9, 9}, {6, 6}};
    placeAll(b, black, Stone::Black);
    placeAll(b, white, Stone::White);
    placeAll(swapped, black, Stone::White);
    placeAll(swapped, white, Stone::Black);
    EXPECT_EQ(ai::Evaluator::evaluate(b, Stone::Black), ai::Evaluator::evaluate(swapped, Stone::White));
    EXPECT_EQ(ai::Evaluator::evaluate(b, Stone::White), ai::Evaluator::evaluate(swapped, Stone::Black));
}

TEST(Evaluator, ShortDiagonalIgnored) {
    Board b;
    // (0,3)(1,2)(2,1)(3,0) 這條反斜線只有 4 格；其他方向都是單子
    placeAll(b, {{0, 3}, {1, 2}, {2, 1}, {3, 0}}, Stone::Black);
    EXPECT_EQ(ai::Evaluator::evaluate(b, Stone::Black), 0);
}
