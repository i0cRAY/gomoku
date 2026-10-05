#include <gtest/gtest.h>

#include <vector>

#include "GameState.h"
#include "ReplayCursor.h"

using namespace core;

namespace {

std::vector<Move> threeMoves() {
    return {{{7, 7}, Stone::Black, 0}, {{7, 8}, Stone::White, 0}, {{8, 8}, Stone::Black, 0}};
}

void expectSameBoard(const Board& a, const Board& b) {
    for (int r = 0; r < Board::kSize; ++r) {
        for (int c = 0; c < Board::kSize; ++c) {
            ASSERT_EQ(a.at({r, c}), b.at({r, c})) << "at (" << r << "," << c << ")";
        }
    }
    EXPECT_EQ(a.stoneCount(), b.stoneCount());
    EXPECT_EQ(a.hash(), b.hash());
}

} // namespace

TEST(ReplayCursor, EmptyRecordCannotMove) {
    ReplayCursor cur({});
    EXPECT_EQ(cur.index(), 0u);
    EXPECT_EQ(cur.size(), 0u);
    EXPECT_EQ(cur.lastMove(), std::nullopt);
    EXPECT_FALSE(cur.next());
    EXPECT_FALSE(cur.prev());
    EXPECT_FALSE(cur.toStart());
    EXPECT_FALSE(cur.toEnd());
    EXPECT_EQ(cur.board().stoneCount(), 0);
}

TEST(ReplayCursor, StartsAtEmptyBoard) {
    ReplayCursor cur(threeMoves());
    EXPECT_EQ(cur.index(), 0u);
    EXPECT_EQ(cur.size(), 3u);
    EXPECT_EQ(cur.board().stoneCount(), 0);
    EXPECT_EQ(cur.lastMove(), std::nullopt);
}

TEST(ReplayCursor, NextPlacesMovesInOrder) {
    ReplayCursor cur(threeMoves());
    ASSERT_TRUE(cur.next());
    EXPECT_EQ(cur.index(), 1u);
    EXPECT_EQ(cur.board().at({7, 7}), Stone::Black);
    EXPECT_EQ(cur.lastMove(), (Pos{7, 7}));

    ASSERT_TRUE(cur.next());
    EXPECT_EQ(cur.index(), 2u);
    EXPECT_EQ(cur.board().at({7, 8}), Stone::White);
    EXPECT_EQ(cur.lastMove(), (Pos{7, 8}));
}

TEST(ReplayCursor, NextAtEndDoesNothing) {
    ReplayCursor cur(threeMoves());
    cur.toEnd();
    EXPECT_FALSE(cur.next());
    EXPECT_EQ(cur.index(), 3u);
    EXPECT_EQ(cur.board().stoneCount(), 3);
}

TEST(ReplayCursor, PrevRemovesLatestMove) {
    ReplayCursor cur(threeMoves());
    cur.next();
    cur.next();
    ASSERT_TRUE(cur.prev());
    EXPECT_EQ(cur.index(), 1u);
    EXPECT_EQ(cur.board().at({7, 8}), Stone::Empty);
    EXPECT_EQ(cur.board().at({7, 7}), Stone::Black);
    EXPECT_EQ(cur.lastMove(), (Pos{7, 7}));

    ASSERT_TRUE(cur.prev());
    EXPECT_EQ(cur.index(), 0u);
    EXPECT_EQ(cur.lastMove(), std::nullopt);
}

TEST(ReplayCursor, PrevAtStartDoesNothing) {
    ReplayCursor cur(threeMoves());
    EXPECT_FALSE(cur.prev());
    EXPECT_EQ(cur.index(), 0u);
}

TEST(ReplayCursor, ToEndShowsAllMoves) {
    ReplayCursor cur(threeMoves());
    ASSERT_TRUE(cur.toEnd());
    EXPECT_EQ(cur.index(), 3u);
    EXPECT_EQ(cur.board().stoneCount(), 3);
    EXPECT_EQ(cur.lastMove(), (Pos{8, 8}));
    EXPECT_FALSE(cur.toEnd());
}

TEST(ReplayCursor, ToStartClearsBoard) {
    ReplayCursor cur(threeMoves());
    cur.toEnd();
    ASSERT_TRUE(cur.toStart());
    EXPECT_EQ(cur.index(), 0u);
    EXPECT_EQ(cur.board().stoneCount(), 0);
    EXPECT_EQ(cur.lastMove(), std::nullopt);
    EXPECT_FALSE(cur.toStart());
}

TEST(ReplayCursor, BoardMatchesPlayedGameAtEveryIndex) {
    const std::vector<Pos> cells{{7, 7}, {7, 8}, {8, 8}, {6, 6}, {9, 9}, {0, 0}, {14, 14}};
    std::vector<Board> expected{Board{}};
    GameState g;
    for (const Pos p : cells) {
        ASSERT_TRUE(g.play(p, 0));
        expected.push_back(g.board());
    }
    ReplayCursor cur(g.history().moves());

    for (std::size_t i = 0; i < expected.size(); ++i) {
        SCOPED_TRACE(i);
        expectSameBoard(cur.board(), expected[i]);
        cur.next();
    }
    for (std::size_t i = expected.size(); i-- > 0;) {
        SCOPED_TRACE(i);
        expectSameBoard(cur.board(), expected[i]);
        cur.prev();
    }
}
