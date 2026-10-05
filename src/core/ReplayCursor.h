#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "Board.h"
#include "Types.h"

namespace core {

// 回放時在棋步之間移動，不修改棋譜（SDD §5.1）。
// 前置條件：moves 已驗證合法（例如來自 restore() 後的 GameState::history()）。
class ReplayCursor {
public:
    explicit ReplayCursor(std::vector<Move> moves);

    std::size_t index() const;   // 目前顯示前幾手，0 = 空盤
    std::size_t size() const;    // 總手數
    const Board& board() const;
    std::optional<Pos> lastMove() const;

    // 以下皆回傳是否有移動；已在邊界時不動
    bool toStart();
    bool prev();
    bool next();
    bool toEnd();

private:
    std::vector<Move> m_moves;
    Board m_board;
    std::size_t m_index = 0;
};

} // namespace core
