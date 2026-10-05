#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "GameState.h"
#include "Types.h"

namespace core {

enum class MatchType { Local, HumanVsAi, AiVsAi, Lan };
enum class PlayerType { Human, Ai };
enum class Difficulty { Easy, Normal, Hard };

struct PlayerInfo {
    PlayerType type;
    std::optional<Difficulty> difficulty;   // 只有 Ai 有

    bool operator==(const PlayerInfo&) const = default;
};

struct TimeControl {
    bool timed;                 // false = "none"，true = "per_move"
    std::int64_t moveLimitMs;   // 非限時為 0

    bool operator==(const TimeControl&) const = default;
};

// 存檔／棋譜的內容（SDD §5.1、§6.1）。JSON 轉換在 format 模組。
struct GameRecord {
    std::string createdAt;                     // ISO 8601，由呼叫端提供
    MatchType matchType = MatchType::Local;
    PlayerInfo black{PlayerType::Human, std::nullopt};
    PlayerInfo white{PlayerType::Human, std::nullopt};
    TimeControl timeControl{false, 0};
    std::vector<Move> moves;                   // color 不採用，重播時依黑先輪流推得
    std::optional<std::int64_t> remainingMs;   // 限時且未結束時才有
    GameResult result = GameResult::Ongoing;
    ResultReason reason = ResultReason::None;
};

// 從空盤重播並依 SDD §6.1 驗證；成功回傳重建的 GameState，失敗回傳錯誤訊息（含第幾手）
std::variant<GameState, std::string> restore(const GameRecord& r);

} // namespace core
