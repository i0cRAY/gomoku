#include <gtest/gtest.h>

#include "Board.h"

using namespace core;

TEST(Board, StartsEmpty) {
    Board b;
    for (int r = 0; r < Board::kSize; ++r) {
        for (int c = 0; c < Board::kSize; ++c) {
            EXPECT_EQ(b.at({r, c}), Stone::Empty);
        }
    }
    EXPECT_EQ(b.stoneCount(), 0);
}

TEST(Board, InBounds) {
    Board b;
    EXPECT_TRUE(b.inBounds({0, 0}));
    EXPECT_TRUE(b.inBounds({Board::kSize - 1, Board::kSize - 1}));
    EXPECT_FALSE(b.inBounds({-1, 0}));
    EXPECT_FALSE(b.inBounds({0, Board::kSize}));
    EXPECT_FALSE(b.inBounds({Board::kSize, 0}));
}

TEST(Board, PlaceAndRemove) {
    Board b;
    b.place({7, 7}, Stone::Black);
    EXPECT_EQ(b.at({7, 7}), Stone::Black);
    EXPECT_FALSE(b.isEmpty({7, 7}));
    EXPECT_EQ(b.stoneCount(), 1);

    b.remove({7, 7});
    EXPECT_EQ(b.at({7, 7}), Stone::Empty);
    EXPECT_TRUE(b.isEmpty({7, 7}));
    EXPECT_EQ(b.stoneCount(), 0);
}

TEST(Board, ZobristHashReturnsToOriginalAfterPlaceAndRemove) {
    Board b;
    const auto original = b.hash();
    b.place({3, 4}, Stone::White);
    EXPECT_NE(b.hash(), original);
    b.remove({3, 4});
    EXPECT_EQ(b.hash(), original);
}

TEST(Board, ZobristHashIndependentOfPlacementOrder) {
    Board a;
    a.place({0, 0}, Stone::Black);
    a.place({1, 1}, Stone::White);

    Board b;
    b.place({1, 1}, Stone::White);
    b.place({0, 0}, Stone::Black);

    EXPECT_EQ(a.hash(), b.hash());
}

TEST(Board, ZobristHashDiffersForDifferentBoards) {
    Board a;
    a.place({0, 0}, Stone::Black);

    Board b;
    b.place({0, 0}, Stone::White);

    EXPECT_NE(a.hash(), b.hash());
}

// Precondition violations abort via assert, which only exists in debug builds.
class BoardDeathTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef NDEBUG
        GTEST_SKIP() << "asserts are disabled in release builds";
#endif
    }
};

TEST_F(BoardDeathTest, AtOutOfBoundsAborts) {
    Board b;
    EXPECT_DEATH(b.at({-1, 0}), "");
    EXPECT_DEATH(b.at({0, Board::kSize}), "");
}

TEST_F(BoardDeathTest, PlaceOutOfBoundsAborts) {
    Board b;
    EXPECT_DEATH(b.place({Board::kSize, 0}, Stone::Black), "");
}

TEST_F(BoardDeathTest, PlaceOnOccupiedAborts) {
    Board b;
    b.place({7, 7}, Stone::Black);
    EXPECT_DEATH(b.place({7, 7}, Stone::White), "");
}

TEST_F(BoardDeathTest, PlaceEmptyStoneAborts) {
    Board b;
    EXPECT_DEATH(b.place({7, 7}, Stone::Empty), "");
}

TEST_F(BoardDeathTest, RemoveFromEmptyCellAborts) {
    Board b;
    EXPECT_DEATH(b.remove({7, 7}), "");
}

TEST_F(BoardDeathTest, RemoveOutOfBoundsAborts) {
    Board b;
    EXPECT_DEATH(b.remove({0, -1}), "");
}
