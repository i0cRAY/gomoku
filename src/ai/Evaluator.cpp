#include "Evaluator.h"

#include <array>
#include <cstdint>

namespace ai::Evaluator {

namespace {

using core::Board;
using core::Pos;
using core::Stone;

constexpr int kMinSegment = 5;
constexpr int kMaxSegment = Board::kSize;

// ---- 棋組在區段內的分類 ----
// 棋組以位元遮罩表示（第 i 位 = 區段第 i 格有己方棋子），其他棋組視為空點（SDD §5.2.2）。
// 判定只取決於（區段長度, 遮罩），全部預先計算成表，約 6.5 萬筆。

bool hasFive(std::uint32_t mask) {
    int run = 0;
    for (; mask != 0; mask >>= 1) {
        run = (mask & 1u) ? run + 1 : 0;
        if (run >= 5) {
            return true;
        }
    }
    return false;
}

class PatternTable {
public:
    PatternTable() {
        int offset = 0;
        for (int len = kMinSegment; len <= kMaxSegment; ++len) {
            m_offset[len] = offset;
            offset += 1 << len;
        }
        m_table.assign(offset, kUnknown);
        for (int len = kMinSegment; len <= kMaxSegment; ++len) {
            for (std::uint32_t mask = 0; mask < (1u << len); ++mask) {
                classify(len, mask);
            }
        }
    }

    Pattern at(int len, std::uint32_t mask) const {
        return static_cast<Pattern>(m_table[m_offset[len] + mask]);
    }

private:
    static constexpr std::uint8_t kUnknown = 0xFF;

    Pattern classify(int len, std::uint32_t mask) {
        std::uint8_t& slot = m_table[m_offset[len] + mask];
        if (slot == kUnknown) {
            slot = static_cast<std::uint8_t>(compute(len, mask));
        }
        return static_cast<Pattern>(slot);
    }

    Pattern compute(int len, std::uint32_t mask) {
        if (hasFive(mask)) {
            return Pattern::Five;
        }
        int fivePoints = 0;
        for (int p = 0; p < len; ++p) {
            if (!(mask & (1u << p)) && hasFive(mask | (1u << p))) {
                ++fivePoints;
            }
        }
        if (fivePoints >= 2) {
            return Pattern::OpenFour;
        }
        if (fivePoints == 1) {
            return Pattern::Four;
        }
        // 放一子後可達到的最強棋型（遞迴查表；棋子數只會增加，必定終止）
        Pattern bestAfter = Pattern::None;
        for (int p = 0; p < len; ++p) {
            if (!(mask & (1u << p))) {
                const Pattern after = classify(len, mask | (1u << p));
                if (after > bestAfter) {
                    bestAfter = after;
                }
            }
        }
        switch (bestAfter) {
            case Pattern::OpenFour:
                return Pattern::OpenThree;
            case Pattern::Four:
                return Pattern::Three;
            case Pattern::OpenThree:
                return Pattern::OpenTwo;
            case Pattern::Three:
                return Pattern::Two;
            default:
                return Pattern::None;
        }
    }

    std::array<int, kMaxSegment + 1> m_offset{};
    std::vector<std::uint8_t> m_table;
};

const PatternTable& patternTable() {
    static const PatternTable table;   // 第一次使用時建立；C++11 起保證執行緒安全
    return table;
}

// 對 line 上 side 的每個棋組呼叫 visit(pattern)
template <typename Visit>
void forEachGroup(const Stone* line, int len, Stone side, Visit visit) {
    const PatternTable& table = patternTable();
    const Stone opp = core::opponent(side);
    int segStart = 0;
    for (int i = 0; i <= len; ++i) {
        if (i < len && line[i] != opp) {
            continue;
        }
        // 區段 [segStart, i)
        const int segLen = i - segStart;
        if (segLen >= kMinSegment) {
            std::uint32_t group = 0;
            int lastStone = 0;
            for (int j = 0; j < segLen; ++j) {
                if (line[segStart + j] != side) {
                    continue;
                }
                if (group != 0 && j - lastStone > 2) {   // 相隔超過 1 個空點，換下一組
                    visit(table.at(segLen, group));
                    group = 0;
                }
                group |= 1u << j;
                lastStone = j;
            }
            if (group != 0) {
                visit(table.at(segLen, group));
            }
        }
        segStart = i + 1;
    }
}

// 四個方向：橫、直、左上到右下、右上到左下
constexpr std::array<Pos, 4> kDirections{{{0, 1}, {1, 0}, {1, 1}, {1, -1}}};

// 對棋盤上每條長度 ≥ 5 的線呼叫 visit(stones, len)
template <typename Visit>
void forEachLine(const Board& b, Visit visit) {
    std::array<Stone, Board::kSize> buf{};
    for (const Pos d : kDirections) {
        for (int r = 0; r < Board::kSize; ++r) {
            for (int c = 0; c < Board::kSize; ++c) {
                // 只從線的起點開始：前一格在棋盤外
                if (b.inBounds({r - d.row, c - d.col})) {
                    continue;
                }
                int len = 0;
                for (Pos p{r, c}; b.inBounds(p); p = {p.row + d.row, p.col + d.col}) {
                    buf[len++] = b.at(p);
                }
                if (len >= kMinSegment) {
                    visit(buf.data(), len);
                }
            }
        }
    }
}

} // namespace

int patternScore(Pattern p) {
    switch (p) {
        case Pattern::Five:
            return 10'000'000;
        case Pattern::OpenFour:
            return 100'000;
        case Pattern::Four:
            return 10'000;
        case Pattern::OpenThree:
            return 5'000;
        case Pattern::Three:
            return 500;
        case Pattern::OpenTwo:
            return 200;
        case Pattern::Two:
            return 50;
        case Pattern::None:
            return 0;
    }
    return 0;
}

int sideScore(const Board& b, Stone side) {
    int total = 0;
    forEachLine(b, [&](const Stone* line, int len) {
        forEachGroup(line, len, side, [&](Pattern p) { total += patternScore(p); });
    });
    return total;
}

int evaluate(const Board& b, Stone side) {
    return sideScore(b, side) - sideScore(b, core::opponent(side)) * 11 / 10;
}

std::vector<Pattern> patternsInLine(const std::vector<Stone>& line, Stone side) {
    std::vector<Pattern> out;
    forEachGroup(line.data(), static_cast<int>(line.size()), side, [&](Pattern p) {
        if (p != Pattern::None) {
            out.push_back(p);
        }
    });
    return out;
}

} // namespace ai::Evaluator
