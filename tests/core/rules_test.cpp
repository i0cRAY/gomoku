#include <gtest/gtest.h>

#include "Board.h"
#include "Rules.h"

using namespace core;

TEST(Rules, IsLegalRejectsOutOfBounds) {
    Board b;
    EXPECT_FALSE(Rules::isLegal(b, {-1, 0}));
    EXPECT_FALSE(Rules::isLegal(b, {0, Board::kSize}));
}

TEST(Rules, IsLegalRejectsOccupied) {
    Board b;
    b.place({7, 7}, Stone::Black);
    EXPECT_FALSE(Rules::isLegal(b, {7, 7}));
}

TEST(Rules, IsLegalAcceptsEmptyInBounds) {
    Board b;
    EXPECT_TRUE(Rules::isLegal(b, {7, 7}));
}

TEST(Rules, MakesFiveHorizontal) {
    Board b;
    for (int c = 3; c <= 7; ++c) {
        b.place({5, c}, Stone::Black);
    }
    EXPECT_TRUE(Rules::makesFive(b, {5, 7}));
}

TEST(Rules, MakesFiveVertical) {
    Board b;
    for (int r = 3; r <= 7; ++r) {
        b.place({r, 5}, Stone::White);
    }
    EXPECT_TRUE(Rules::makesFive(b, {7, 5}));
}

TEST(Rules, MakesFiveDiagonalDown) {
    Board b;
    for (int i = 0; i < 5; ++i) {
        b.place({3 + i, 3 + i}, Stone::Black);
    }
    EXPECT_TRUE(Rules::makesFive(b, {7, 7}));
}

TEST(Rules, MakesFiveDiagonalUp) {
    Board b;
    for (int i = 0; i < 5; ++i) {
        b.place({3 + i, 7 - i}, Stone::White);
    }
    EXPECT_TRUE(Rules::makesFive(b, {7, 3}));
}

TEST(Rules, SixInRowAlsoCounts) {
    Board b;
    for (int c = 2; c <= 7; ++c) {
        b.place({5, c}, Stone::Black);
    }
    EXPECT_TRUE(Rules::makesFive(b, {5, 4}));
}

TEST(Rules, FourInRowIsNotFive) {
    Board b;
    for (int c = 3; c <= 6; ++c) {
        b.place({5, c}, Stone::Black);
    }
    EXPECT_FALSE(Rules::makesFive(b, {5, 6}));
}

TEST(Rules, MakesFiveAtBoardEdgeAndCorner) {
    Board b;
    for (int c = 0; c <= 4; ++c) {
        b.place({0, c}, Stone::Black);
    }
    EXPECT_TRUE(Rules::makesFive(b, {0, 0}));
}

TEST(Rules, IsFullDetectsFullBoard) {
    Board b;
    EXPECT_FALSE(Rules::isFull(b));
    for (int r = 0; r < Board::kSize; ++r) {
        for (int c = 0; c < Board::kSize; ++c) {
            b.place({r, c}, (r + c) % 2 == 0 ? Stone::Black : Stone::White);
        }
    }
    EXPECT_TRUE(Rules::isFull(b));
}
