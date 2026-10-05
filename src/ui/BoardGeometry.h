#pragma once

#include <optional>

#include "Types.h"

// 棋盤在元件中的像素配置。純計算、不依賴 Qt，方便單元測試。
// 棋盤取元件較短邊為邊長並置中；四周各留一格格距作為邊距，
// 所以邊長 = 16 格（14 格格線 + 兩側邊距）。
class BoardGeometry {
public:
    struct Point {
        double x;
        double y;
    };

    // 點擊位置離最近交叉點超過格距的這個比例就忽略（SDD §5.4）
    static constexpr double kHitRadiusRatio = 0.4;

    BoardGeometry(int width, int height);

    double cellSize() const;
    Point center(core::Pos p) const;                       // row 對應 y，col 對應 x
    std::optional<core::Pos> hitTest(double x, double y) const;

private:
    double m_cellSize = 0.0;
    Point m_origin{0.0, 0.0};   // 交叉點 (0,0) 的像素座標
};
