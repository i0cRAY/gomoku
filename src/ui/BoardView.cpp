#include "BoardView.h"

#include <array>

#include <QMouseEvent>
#include <QPainter>

#include "BoardGeometry.h"

namespace {

const QColor kBoardColor(0xDC, 0xB3, 0x5C);
const QColor kLineColor(0x3A, 0x2A, 0x10);
const QColor kLastMoveColor(0xD0, 0x30, 0x30);

constexpr double kStoneRadiusRatio = 0.45;
constexpr double kStarRadiusRatio = 0.12;
constexpr double kLastMoveRadiusRatio = 0.12;

constexpr std::array<core::Pos, 5> kStarPoints{{{3, 3}, {3, 11}, {7, 7}, {11, 3}, {11, 11}}};

QPointF toQt(BoardGeometry::Point p) {
    return {p.x, p.y};
}

} // namespace

BoardView::BoardView(QWidget* parent) : QWidget(parent) {
    setMinimumSize(240, 240);
}

void BoardView::setBoard(const core::Board& board, std::optional<core::Pos> lastMove) {
    m_board = board;
    m_lastMove = lastMove;
    update();
}

void BoardView::setInteractive(bool interactive) {
    m_interactive = interactive;
}

QSize BoardView::sizeHint() const {
    return {640, 640};
}

void BoardView::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), kBoardColor);

    const BoardGeometry geo(width(), height());
    const double cell = geo.cellSize();
    constexpr int kLast = core::Board::kSize - 1;

    painter.setPen(QPen(kLineColor, 1.0));
    for (int i = 0; i <= kLast; ++i) {
        painter.drawLine(toQt(geo.center({i, 0})), toQt(geo.center({i, kLast})));
        painter.drawLine(toQt(geo.center({0, i})), toQt(geo.center({kLast, i})));
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(kLineColor);
    for (const core::Pos p : kStarPoints) {
        painter.drawEllipse(toQt(geo.center(p)), cell * kStarRadiusRatio, cell * kStarRadiusRatio);
    }

    for (int row = 0; row <= kLast; ++row) {
        for (int col = 0; col <= kLast; ++col) {
            const core::Stone s = m_board.at({row, col});
            if (s == core::Stone::Empty) {
                continue;
            }
            painter.setPen(s == core::Stone::White ? QPen(Qt::black, 1.0) : QPen(Qt::NoPen));
            painter.setBrush(s == core::Stone::Black ? QColor(0x10, 0x10, 0x10) : QColor(0xF8, 0xF8, 0xF8));
            painter.drawEllipse(toQt(geo.center({row, col})), cell * kStoneRadiusRatio, cell * kStoneRadiusRatio);
        }
    }

    if (m_lastMove) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(kLastMoveColor);
        painter.drawEllipse(toQt(geo.center(*m_lastMove)), cell * kLastMoveRadiusRatio,
                            cell * kLastMoveRadiusRatio);
    }
}

void BoardView::mousePressEvent(QMouseEvent* event) {
    if (!m_interactive || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    const BoardGeometry geo(width(), height());
    if (const auto p = geo.hitTest(event->position().x(), event->position().y())) {
        emit cellClicked(*p);
    }
}
