#include "Candidates.h"

#include <algorithm>
#include <array>

#include "Evaluator.h"

namespace ai {

using core::Board;
using core::Pos;
using core::Stone;

std::vector<Pos> generateCandidates(const Board& b, Stone side, int radius, int maxCount) {
    constexpr int kCenter = Board::kSize / 2;
    if (b.stoneCount() == 0) {
        return {{kCenter, kCenter}};
    }

    std::array<bool, Board::kSize * Board::kSize> near{};
    for (int r = 0; r < Board::kSize; ++r) {
        for (int c = 0; c < Board::kSize; ++c) {
            if (b.isEmpty({r, c})) {
                continue;
            }
            for (int dr = -radius; dr <= radius; ++dr) {
                for (int dc = -radius; dc <= radius; ++dc) {
                    const Pos q{r + dr, c + dc};
                    if (b.inBounds(q) && b.isEmpty(q)) {
                        near[q.row * Board::kSize + q.col] = true;
                    }
                }
            }
        }
    }

    struct Scored {
        Pos pos;
        int score;
    };
    std::vector<Scored> scored;
    const Stone opp = core::opponent(side);
    for (int r = 0; r < Board::kSize; ++r) {   // 依 row、col 順序加入，stable_sort 保留同分順序
        for (int c = 0; c < Board::kSize; ++c) {
            if (near[r * Board::kSize + c]) {
                const Pos p{r, c};
                scored.push_back({p, Evaluator::pointGain(b, p, side) + Evaluator::pointGain(b, p, opp)});
            }
        }
    }
    std::stable_sort(scored.begin(), scored.end(), [](const Scored& a, const Scored& b) { return a.score > b.score; });
    if (static_cast<int>(scored.size()) > maxCount) {
        scored.resize(maxCount);
    }

    std::vector<Pos> out;
    out.reserve(scored.size());
    for (const Scored& s : scored) {
        out.push_back(s.pos);
    }
    return out;
}

} // namespace ai
