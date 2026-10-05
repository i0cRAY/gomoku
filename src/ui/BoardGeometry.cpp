#include "BoardGeometry.h"

#include <algorithm>
#include <cmath>

#include "Board.h"

namespace {

constexpr int kCellsAcross = core::Board::kSize + 1;

} // namespace

BoardGeometry::BoardGeometry(int width, int height) {
    const double side = std::max(0, std::min(width, height));
    m_cellSize = side / kCellsAcross;
    m_origin = {(width - side) / 2.0 + m_cellSize, (height - side) / 2.0 + m_cellSize};
}

double BoardGeometry::cellSize() const {
    return m_cellSize;
}

BoardGeometry::Point BoardGeometry::center(core::Pos p) const {
    return {m_origin.x + p.col * m_cellSize, m_origin.y + p.row * m_cellSize};
}

std::optional<core::Pos> BoardGeometry::hitTest(double x, double y) const {
    if (m_cellSize <= 0.0) {
        return std::nullopt;
    }
    const core::Pos p{static_cast<int>(std::lround((y - m_origin.y) / m_cellSize)),
                      static_cast<int>(std::lround((x - m_origin.x) / m_cellSize))};
    if (p.row < 0 || p.row >= core::Board::kSize || p.col < 0 || p.col >= core::Board::kSize) {
        return std::nullopt;
    }
    const Point c = center(p);
    if (std::hypot(x - c.x, y - c.y) > kHitRadiusRatio * m_cellSize) {
        return std::nullopt;
    }
    return p;
}
