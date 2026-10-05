#include <gtest/gtest.h>

#include "BoardGeometry.h"

using core::Pos;

// 160x160：棋盤邊長 160，邊距與格距皆為 160 / 16 = 10，(0,0) 在像素 (10,10)。

TEST(BoardGeometry, CellSizeUsesShorterSide) {
    EXPECT_DOUBLE_EQ(BoardGeometry(160, 160).cellSize(), 10.0);
    EXPECT_DOUBLE_EQ(BoardGeometry(260, 160).cellSize(), 10.0);
    EXPECT_DOUBLE_EQ(BoardGeometry(160, 320).cellSize(), 10.0);
}

TEST(BoardGeometry, CenterOfCorners) {
    BoardGeometry g(160, 160);
    EXPECT_DOUBLE_EQ(g.center({0, 0}).x, 10.0);
    EXPECT_DOUBLE_EQ(g.center({0, 0}).y, 10.0);
    EXPECT_DOUBLE_EQ(g.center({14, 14}).x, 150.0);
    EXPECT_DOUBLE_EQ(g.center({14, 14}).y, 150.0);
}

TEST(BoardGeometry, CenterMapsRowToYAndColToX) {
    BoardGeometry g(160, 160);
    EXPECT_DOUBLE_EQ(g.center({2, 5}).x, 60.0);
    EXPECT_DOUBLE_EQ(g.center({2, 5}).y, 30.0);
}

TEST(BoardGeometry, BoardIsCenteredInWideWidget) {
    BoardGeometry g(260, 160);
    EXPECT_DOUBLE_EQ(g.center({0, 0}).x, 60.0);
    EXPECT_DOUBLE_EQ(g.center({0, 0}).y, 10.0);
}

TEST(BoardGeometry, BoardIsCenteredInTallWidget) {
    BoardGeometry g(160, 260);
    EXPECT_DOUBLE_EQ(g.center({0, 0}).x, 10.0);
    EXPECT_DOUBLE_EQ(g.center({0, 0}).y, 60.0);
}

TEST(BoardGeometry, HitTestExactIntersections) {
    BoardGeometry g(160, 160);
    EXPECT_EQ(g.hitTest(10, 10), (Pos{0, 0}));
    EXPECT_EQ(g.hitTest(150, 10), (Pos{0, 14}));
    EXPECT_EQ(g.hitTest(10, 150), (Pos{14, 0}));
    EXPECT_EQ(g.hitTest(150, 150), (Pos{14, 14}));
    EXPECT_EQ(g.hitTest(80, 80), (Pos{7, 7}));
}

TEST(BoardGeometry, HitTestSnapsToNearestIntersection) {
    BoardGeometry g(160, 160);
    EXPECT_EQ(g.hitTest(82, 78), (Pos{7, 7}));
    EXPECT_EQ(g.hitTest(58, 32), (Pos{2, 5}));
}

TEST(BoardGeometry, HitTestAcceptsExactlyFortyPercent) {
    BoardGeometry g(160, 160);
    EXPECT_EQ(g.hitTest(14, 10), (Pos{0, 0}));
    EXPECT_EQ(g.hitTest(80, 76), (Pos{7, 7}));
}

TEST(BoardGeometry, HitTestRejectsBeyondFortyPercent) {
    BoardGeometry g(160, 160);
    EXPECT_EQ(g.hitTest(14.1, 10), std::nullopt);
    EXPECT_EQ(g.hitTest(85, 80), std::nullopt);   // 兩條線正中間
    EXPECT_EQ(g.hitTest(83, 83), std::nullopt);   // 各軸 3 但直線距離約 4.24
}

TEST(BoardGeometry, HitTestRejectsOutsideBoard) {
    BoardGeometry g(160, 160);
    EXPECT_EQ(g.hitTest(0, 0), std::nullopt);
    EXPECT_EQ(g.hitTest(160, 150), std::nullopt);   // 最近的「交叉點」會是第 15 列
    EXPECT_EQ(g.hitTest(-10, 80), std::nullopt);
}

TEST(BoardGeometry, HitTestInWideWidgetUsesOffset) {
    BoardGeometry g(260, 160);
    EXPECT_EQ(g.hitTest(60, 10), (Pos{0, 0}));
    EXPECT_EQ(g.hitTest(10, 10), std::nullopt);   // 左側空白處
}

TEST(BoardGeometry, ZeroSizeWidgetHitsNothing) {
    BoardGeometry g(0, 0);
    EXPECT_EQ(g.hitTest(0, 0), std::nullopt);
}
