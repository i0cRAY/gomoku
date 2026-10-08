#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "Board.h"
#include "Candidates.h"

using namespace core;
using ai::generateCandidates;

namespace {

std::vector<Pos> sorted(std::vector<Pos> v) {
    std::sort(v.begin(), v.end(), [](Pos a, Pos b) { return a.row != b.row ? a.row < b.row : a.col < b.col; });
    return v;
}

} // namespace

TEST(Candidates, EmptyBoardOnlyCenter) {
    Board b;
    EXPECT_EQ(generateCandidates(b, Stone::Black, 2, 20), (std::vector<Pos>{{7, 7}}));
}

TEST(Candidates, OnlyEmptyPointsWithinRadius) {
    Board b;
    b.place({7, 7}, Stone::Black);
    const auto r1 = sorted(generateCandidates(b, Stone::White, 1, 100));
    EXPECT_EQ(r1, (std::vector<Pos>{{6, 6}, {6, 7}, {6, 8}, {7, 6}, {7, 8}, {8, 6}, {8, 7}, {8, 8}}));
    EXPECT_EQ(generateCandidates(b, Stone::White, 2, 100).size(), 24u);   // 5×5 − 1
}

TEST(Candidates, ClippedAtBoardEdge) {
    Board b;
    b.place({0, 0}, Stone::Black);
    EXPECT_EQ(generateCandidates(b, Stone::White, 2, 100).size(), 8u);   // 3×3 − 1
}

TEST(Candidates, NoDuplicatesAndNoOccupied) {
    Board b;
    b.place({7, 7}, Stone::Black);
    b.place({7, 8}, Stone::White);
    const auto c = generateCandidates(b, Stone::Black, 2, 100);
    EXPECT_EQ(c.size(), 5u * 6u - 2u);   // 兩子周圍 5×6 範圍，扣掉兩顆棋子
    EXPECT_EQ(std::count(c.begin(), c.end(), Pos{7, 7}), 0);
    EXPECT_EQ(std::count(c.begin(), c.end(), Pos{7, 8}), 0);
    const auto s = sorted(c);
    EXPECT_EQ(std::adjacent_find(s.begin(), s.end()), s.end());
}

TEST(Candidates, MaxCountLimitsResult) {
    Board b;
    b.place({7, 7}, Stone::Black);
    EXPECT_EQ(generateCandidates(b, Stone::White, 2, 8).size(), 8u);
    EXPECT_EQ(generateCandidates(b, Stone::White, 2, 1).size(), 1u);
}

TEST(Candidates, OrderedByScoreThenRowThenCol) {
    Board b;
    b.place({7, 7}, Stone::Black);
    // 與 (7,7) 同線、距離 1 或 2 的 16 點都形成活二（200 分），其餘 8 點為 0 分；同分依 row、col
    const std::vector<Pos> expected{
        {5, 5}, {5, 7}, {5, 9}, {6, 6}, {6, 7}, {6, 8}, {7, 5}, {7, 6},
        {7, 8}, {7, 9}, {8, 6}, {8, 7}, {8, 8}, {9, 5}, {9, 7}, {9, 9},
        {5, 6}, {5, 8}, {6, 5}, {6, 9}, {8, 5}, {8, 9}, {9, 6}, {9, 8},
    };
    EXPECT_EQ(generateCandidates(b, Stone::White, 2, 100), expected);
}

TEST(Candidates, BlockingPointsOfOpenThreeComeFirst) {
    Board b;
    for (int c = 6; c <= 8; ++c) {
        b.place({7, c}, Stone::Black);
    }
    b.place({3, 3}, Stone::White);
    const auto c = generateCandidates(b, Stone::White, 2, 20);
    ASSERT_GE(c.size(), 2u);
    EXPECT_EQ(c[0], (Pos{7, 5}));
    EXPECT_EQ(c[1], (Pos{7, 9}));
}

TEST(Candidates, FivePointComesFirst) {
    Board b;
    for (int c = 4; c <= 7; ++c) {
        b.place({7, c}, Stone::Black);
    }
    b.place({7, 3}, Stone::White);
    b.place({8, 8}, Stone::White);
    const auto c = generateCandidates(b, Stone::Black, 2, 20);
    ASSERT_FALSE(c.empty());
    EXPECT_EQ(c[0], (Pos{7, 8}));
}

TEST(Candidates, FullBoardHasNoCandidates) {
    Board b;
    for (int r = 0; r < Board::kSize; ++r) {
        for (int c = 0; c < Board::kSize; ++c) {
            b.place({r, c}, (r + 2 * c) % 4 < 2 ? Stone::Black : Stone::White);
        }
    }
    EXPECT_TRUE(generateCandidates(b, Stone::Black, 2, 20).empty());
}
