#include <gtest/gtest.h>

#include "MoveHistory.h"

using namespace core;

TEST(MoveHistory, PushThenPopReturnsLastMove) {
    MoveHistory h;
    h.push(Move{{0, 0}, Stone::Black, 0});
    h.push(Move{{1, 1}, Stone::White, 100});

    EXPECT_EQ(h.size(), 2u);

    const auto last = h.pop();
    ASSERT_TRUE(last.has_value());
    EXPECT_EQ(last->pos, (Pos{1, 1}));
    EXPECT_EQ(h.size(), 1u);

    const auto first = h.pop();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->pos, (Pos{0, 0}));
    EXPECT_EQ(h.size(), 0u);
}

TEST(MoveHistory, PopOnEmptyReturnsNullopt) {
    MoveHistory h;
    EXPECT_FALSE(h.pop().has_value());
}

TEST(MoveHistory, MovesReturnsInInsertionOrder) {
    MoveHistory h;
    h.push(Move{{0, 0}, Stone::Black, 0});
    h.push(Move{{1, 1}, Stone::White, 0});

    ASSERT_EQ(h.moves().size(), 2u);
    EXPECT_EQ(h.moves()[0].pos, (Pos{0, 0}));
    EXPECT_EQ(h.moves()[1].pos, (Pos{1, 1}));
}
