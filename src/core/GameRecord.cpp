#include "GameRecord.h"

namespace core {

namespace {

bool isWin(GameResult r) {
    return r == GameResult::BlackWin || r == GameResult::WhiteWin;
}

// result 與 reason 的組合是否合理（SDD §6.1 result 表）
bool isConsistent(GameResult r, ResultReason why) {
    switch (why) {
        case ResultReason::None:
            return r == GameResult::Ongoing;
        case ResultReason::BoardFull:
            return r == GameResult::Draw;
        case ResultReason::FiveInRow:
        case ResultReason::Timeout:
        case ResultReason::Resign:
        case ResultReason::Disconnect:
            return isWin(r);
    }
    return false;
}

std::string moveLabel(std::size_t index) {
    return "第 " + std::to_string(index + 1) + " 手";
}

std::string posText(Pos p) {
    return "(" + std::to_string(p.row) + "," + std::to_string(p.col) + ")";
}

} // namespace

std::variant<GameState, std::string> restore(const GameRecord& r) {
    if (!isConsistent(r.result, r.reason)) {
        return std::string("棋譜的勝負結果與原因不一致");
    }

    GameState g;
    for (std::size_t i = 0; i < r.moves.size(); ++i) {
        const Move& m = r.moves[i];
        if (g.result() != GameResult::Ongoing) {
            return moveLabel(i) + "不合法：對局在" + moveLabel(i - 1) + "已經結束";
        }
        if (m.timeUsedMs < 0) {
            return moveLabel(i) + "不合法：用時不可為負數";
        }
        if (!g.board().inBounds(m.pos)) {
            return moveLabel(i) + "不合法：" + posText(m.pos) + " 超出棋盤";
        }
        if (!g.board().isEmpty(m.pos)) {
            return moveLabel(i) + "不合法：" + posText(m.pos) + " 已有棋子";
        }
        g.play(m.pos, m.timeUsedMs);
    }

    switch (r.reason) {
        case ResultReason::None:
            if (g.result() != GameResult::Ongoing) {
                return std::string("棋譜記錄對局尚未結束，但重播後已分出勝負");
            }
            break;
        case ResultReason::FiveInRow:
        case ResultReason::BoardFull:
            if (g.result() != r.result || g.reason() != r.reason) {
                return std::string("棋譜記錄的勝負結果與重播結果不符");
            }
            break;
        case ResultReason::Timeout:
        case ResultReason::Resign:
        case ResultReason::Disconnect:
            // 無法從棋步驗證，只要求重播後對局仍在進行
            if (g.result() != GameResult::Ongoing) {
                return std::string("棋譜記錄的勝負結果與重播結果不符：重播後對局已經結束");
            }
            g.finish(r.result, r.reason);
            break;
    }
    return g;
}

} // namespace core
