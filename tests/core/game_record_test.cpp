#include <gtest/gtest.h>

#include <string>
#include <variant>
#include <vector>

#include "GameRecord.h"

using namespace core;

namespace {

// 本機雙人、不限時的棋譜；棋步顏色依黑先輪流填入
GameRecord makeRecord(const std::vector<Pos>& cells, GameResult result = GameResult::Ongoing,
                      ResultReason reason = ResultReason::None) {
    GameRecord r;
    r.createdAt = "2026-10-05T10:00:00+08:00";
    r.matchType = MatchType::Local;
    r.black = {PlayerType::Human, std::nullopt};
    r.white = {PlayerType::Human, std::nullopt};
    r.timeControl = {false, 0};
    Stone color = Stone::Black;
    for (const Pos p : cells) {
        r.moves.push_back({p, color, 0});
        color = opponent(color);
    }
    r.result = result;
    r.reason = reason;
    return r;
}

// 黑下第 0 列、白下第 1 列，黑在第 9 手完成 (0,0)～(0,4) 連五
std::vector<Pos> blackFiveCells() {
    std::vector<Pos> cells;
    for (int c = 0; c < 4; ++c) {
        cells.push_back({0, c});
        cells.push_back({1, c});
    }
    cells.push_back({0, 4});
    return cells;
}

// 下滿整盤且無人連五，順序黑白交替（同 game_state_test 的 FullBoardIsDraw）
std::vector<Pos> fullBoardDrawCells() {
    std::vector<Pos> black;
    std::vector<Pos> white;
    for (int r = 0; r < Board::kSize; ++r) {
        for (int c = 0; c < Board::kSize; ++c) {
            ((r + 2 * c) % 4 < 2 ? black : white).push_back({r, c});
        }
    }
    std::vector<Pos> cells;
    for (std::size_t i = 0; i < white.size(); ++i) {
        cells.push_back(black[i]);
        cells.push_back(white[i]);
    }
    cells.push_back(black.back());
    return cells;
}

bool errorMentions(const std::variant<GameState, std::string>& out, const std::string& text) {
    const auto* err = std::get_if<std::string>(&out);
    return err != nullptr && err->find(text) != std::string::npos;
}

} // namespace

TEST(GameRecord, EmptyRecordRestoresEmptyGame) {
    const auto out = restore(makeRecord({}));
    ASSERT_TRUE(std::holds_alternative<GameState>(out));
    const auto& g = std::get<GameState>(out);
    EXPECT_EQ(g.board().stoneCount(), 0);
    EXPECT_EQ(g.sideToMove(), Stone::Black);
    EXPECT_EQ(g.result(), GameResult::Ongoing);
}

TEST(GameRecord, LegalMovesRestoreBoardTurnAndHistory) {
    const auto out = restore(makeRecord({{7, 7}, {7, 8}, {8, 8}}));
    ASSERT_TRUE(std::holds_alternative<GameState>(out));
    const auto& g = std::get<GameState>(out);
    EXPECT_EQ(g.board().at({7, 7}), Stone::Black);
    EXPECT_EQ(g.board().at({7, 8}), Stone::White);
    EXPECT_EQ(g.board().at({8, 8}), Stone::Black);
    EXPECT_EQ(g.sideToMove(), Stone::White);
    EXPECT_EQ(g.history().size(), 3u);
    EXPECT_EQ(g.result(), GameResult::Ongoing);
}

TEST(GameRecord, TimeUsedIsPreserved) {
    GameRecord r = makeRecord({{7, 7}, {7, 8}});
    r.moves[0].timeUsedMs = 4210;
    r.moves[1].timeUsedMs = 1830;
    const auto out = restore(r);
    ASSERT_TRUE(std::holds_alternative<GameState>(out));
    const auto& moves = std::get<GameState>(out).history().moves();
    EXPECT_EQ(moves[0].timeUsedMs, 4210);
    EXPECT_EQ(moves[1].timeUsedMs, 1830);
}

TEST(GameRecord, MoveColorsComeFromTurnOrderNotRecord) {
    GameRecord r = makeRecord({{7, 7}, {7, 8}});
    r.moves[0].color = Stone::White;
    r.moves[1].color = Stone::White;
    const auto out = restore(r);
    ASSERT_TRUE(std::holds_alternative<GameState>(out));
    EXPECT_EQ(std::get<GameState>(out).board().at({7, 7}), Stone::Black);
    EXPECT_EQ(std::get<GameState>(out).board().at({7, 8}), Stone::White);
}

TEST(GameRecord, OutOfBoundsMoveRejected) {
    EXPECT_TRUE(errorMentions(restore(makeRecord({{7, 7}, {15, 0}})), "第 2 手"));
    EXPECT_TRUE(errorMentions(restore(makeRecord({{-1, 3}})), "第 1 手"));
}

TEST(GameRecord, OccupiedMoveRejected) {
    EXPECT_TRUE(errorMentions(restore(makeRecord({{7, 7}, {7, 8}, {7, 7}})), "第 3 手"));
}

TEST(GameRecord, NegativeTimeUsedRejected) {
    GameRecord r = makeRecord({{7, 7}, {7, 8}});
    r.moves[1].timeUsedMs = -1;
    EXPECT_TRUE(errorMentions(restore(r), "第 2 手"));
}

TEST(GameRecord, MovesAfterGameOverRejected) {
    auto cells = blackFiveCells();
    cells.push_back({5, 5});
    EXPECT_TRUE(errorMentions(restore(makeRecord(cells, GameResult::BlackWin, ResultReason::FiveInRow)),
                              "第 10 手"));
}

TEST(GameRecord, FiveInRowResultAccepted) {
    const auto out = restore(makeRecord(blackFiveCells(), GameResult::BlackWin, ResultReason::FiveInRow));
    ASSERT_TRUE(std::holds_alternative<GameState>(out));
    EXPECT_EQ(std::get<GameState>(out).result(), GameResult::BlackWin);
    EXPECT_EQ(std::get<GameState>(out).reason(), ResultReason::FiveInRow);
}

TEST(GameRecord, FiveInRowWithWrongWinnerRejected) {
    const auto out = restore(makeRecord(blackFiveCells(), GameResult::WhiteWin, ResultReason::FiveInRow));
    EXPECT_TRUE(std::holds_alternative<std::string>(out));
}

TEST(GameRecord, FiveInRowClaimedWithoutFiveRejected) {
    const auto out = restore(makeRecord({{7, 7}, {7, 8}}, GameResult::WhiteWin, ResultReason::FiveInRow));
    EXPECT_TRUE(std::holds_alternative<std::string>(out));
}

TEST(GameRecord, OngoingRecordThatActuallyEndedRejected) {
    const auto out = restore(makeRecord(blackFiveCells()));
    EXPECT_TRUE(std::holds_alternative<std::string>(out));
}

TEST(GameRecord, BoardFullDrawAccepted) {
    const auto out = restore(makeRecord(fullBoardDrawCells(), GameResult::Draw, ResultReason::BoardFull));
    ASSERT_TRUE(std::holds_alternative<GameState>(out));
    EXPECT_EQ(std::get<GameState>(out).result(), GameResult::Draw);
    EXPECT_EQ(std::get<GameState>(out).reason(), ResultReason::BoardFull);
}

TEST(GameRecord, BoardFullClaimedOnPartialBoardRejected) {
    const auto out = restore(makeRecord({{7, 7}}, GameResult::Draw, ResultReason::BoardFull));
    EXPECT_TRUE(std::holds_alternative<std::string>(out));
}

TEST(GameRecord, TimeoutResultApplied) {
    const auto out = restore(makeRecord({{7, 7}, {7, 8}}, GameResult::WhiteWin, ResultReason::Timeout));
    ASSERT_TRUE(std::holds_alternative<GameState>(out));
    EXPECT_EQ(std::get<GameState>(out).result(), GameResult::WhiteWin);
    EXPECT_EQ(std::get<GameState>(out).reason(), ResultReason::Timeout);
}

TEST(GameRecord, ResignAndDisconnectResultsApplied) {
    const auto resign = restore(makeRecord({}, GameResult::WhiteWin, ResultReason::Resign));
    ASSERT_TRUE(std::holds_alternative<GameState>(resign));
    EXPECT_EQ(std::get<GameState>(resign).reason(), ResultReason::Resign);

    const auto disconnect = restore(makeRecord({{7, 7}}, GameResult::BlackWin, ResultReason::Disconnect));
    ASSERT_TRUE(std::holds_alternative<GameState>(disconnect));
    EXPECT_EQ(std::get<GameState>(disconnect).reason(), ResultReason::Disconnect);
}

TEST(GameRecord, TimeoutAfterGameAlreadyEndedRejected) {
    const auto out = restore(makeRecord(blackFiveCells(), GameResult::WhiteWin, ResultReason::Timeout));
    EXPECT_TRUE(std::holds_alternative<std::string>(out));
}

TEST(GameRecord, InconsistentResultAndReasonRejected) {
    EXPECT_TRUE(std::holds_alternative<std::string>(
        restore(makeRecord({}, GameResult::Ongoing, ResultReason::Timeout))));
    EXPECT_TRUE(std::holds_alternative<std::string>(
        restore(makeRecord({{7, 7}}, GameResult::BlackWin, ResultReason::None))));
    EXPECT_TRUE(std::holds_alternative<std::string>(
        restore(makeRecord({{7, 7}}, GameResult::Draw, ResultReason::Resign))));
    EXPECT_TRUE(std::holds_alternative<std::string>(
        restore(makeRecord(fullBoardDrawCells(), GameResult::BlackWin, ResultReason::BoardFull))));
}
