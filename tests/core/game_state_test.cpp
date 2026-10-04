#include <gtest/gtest.h>

#include <vector>

#include "GameState.h"

using namespace core;

TEST(GameState, BlackMovesFirst) {
    GameState g;
    EXPECT_EQ(g.sideToMove(), Stone::Black);
}

TEST(GameState, TurnsAlternateAfterLegalMove) {
    GameState g;
    ASSERT_TRUE(g.play({7, 7}, 0));
    EXPECT_EQ(g.sideToMove(), Stone::White);
    ASSERT_TRUE(g.play({7, 8}, 0));
    EXPECT_EQ(g.sideToMove(), Stone::Black);
}

TEST(GameState, IllegalMoveRejectedAndTurnUnchanged) {
    GameState g;
    ASSERT_TRUE(g.play({7, 7}, 0));
    EXPECT_FALSE(g.play({7, 7}, 0));    // occupied
    EXPECT_FALSE(g.play({-1, 0}, 0));   // out of bounds
    EXPECT_EQ(g.sideToMove(), Stone::White);
}

TEST(GameState, FiveInRowEndsGame) {
    GameState g;
    for (int c = 0; c < 4; ++c) {
        ASSERT_TRUE(g.play({0, c}, 0));   // black
        ASSERT_TRUE(g.play({1, c}, 0));   // white
    }
    ASSERT_TRUE(g.play({0, 4}, 0));       // black completes five
    EXPECT_EQ(g.result(), GameResult::BlackWin);
    EXPECT_EQ(g.reason(), ResultReason::FiveInRow);
}

TEST(GameState, MovesRejectedAfterGameOver) {
    GameState g;
    for (int c = 0; c < 4; ++c) {
        ASSERT_TRUE(g.play({0, c}, 0));
        ASSERT_TRUE(g.play({1, c}, 0));
    }
    ASSERT_TRUE(g.play({0, 4}, 0));
    EXPECT_FALSE(g.play({2, 2}, 0));
}

TEST(GameState, FullBoardIsDraw) {
    GameState g;

    // (r + 2c) mod 4 caps every run (horizontal/vertical/both diagonals) at
    // length 2, so filling the board this way can never make five-in-a-row.
    std::vector<Pos> blackCells;
    std::vector<Pos> whiteCells;
    for (int r = 0; r < Board::kSize; ++r) {
        for (int c = 0; c < Board::kSize; ++c) {
            if ((r + 2 * c) % 4 < 2) {
                blackCells.push_back({r, c});
            } else {
                whiteCells.push_back({r, c});
            }
        }
    }
    ASSERT_EQ(blackCells.size(), whiteCells.size() + 1);

    for (std::size_t i = 0; i < whiteCells.size(); ++i) {
        ASSERT_TRUE(g.play(blackCells[i], 0));
        ASSERT_TRUE(g.play(whiteCells[i], 0));
    }
    ASSERT_TRUE(g.play(blackCells.back(), 0));

    EXPECT_EQ(g.result(), GameResult::Draw);
    EXPECT_EQ(g.reason(), ResultReason::BoardFull);
}

TEST(GameState, UndoRestoresBoardAndTurn) {
    GameState g;
    ASSERT_TRUE(g.play({7, 7}, 100));   // black
    ASSERT_TRUE(g.play({7, 8}, 200));   // white
    ASSERT_TRUE(g.play({8, 8}, 300));   // black

    ASSERT_TRUE(g.undo(2));

    EXPECT_EQ(g.sideToMove(), Stone::White);
    EXPECT_EQ(g.board().at({7, 8}), Stone::Empty);
    EXPECT_EQ(g.board().at({8, 8}), Stone::Empty);
    EXPECT_EQ(g.board().at({7, 7}), Stone::Black);
    EXPECT_EQ(g.history().size(), 1u);

    ASSERT_EQ(g.lastUndone().size(), 2u);
    EXPECT_EQ(g.lastUndone()[0].pos, (Pos{8, 8}));
    EXPECT_EQ(g.lastUndone()[1].pos, (Pos{7, 8}));
}

TEST(GameState, UndoMoreThanPlayedFails) {
    GameState g;
    ASSERT_TRUE(g.play({7, 7}, 0));
    EXPECT_FALSE(g.undo(2));
}

TEST(GameState, UndoNonPositivePliesFails) {
    GameState g;
    ASSERT_TRUE(g.play({7, 7}, 0));
    EXPECT_FALSE(g.undo(0));
    EXPECT_FALSE(g.undo(-1));
    EXPECT_EQ(g.history().size(), 1u);
}

TEST(GameState, UndoOnEmptyGameFails) {
    GameState g;
    EXPECT_FALSE(g.undo(1));
    EXPECT_EQ(g.sideToMove(), Stone::Black);
}

TEST(GameState, UndoRestoresBoardHash) {
    GameState g;
    ASSERT_TRUE(g.play({7, 7}, 0));
    const auto hashAfterFirstMove = g.board().hash();
    ASSERT_TRUE(g.play({7, 8}, 0));
    ASSERT_TRUE(g.play({8, 8}, 0));

    ASSERT_TRUE(g.undo(2));
    EXPECT_EQ(g.board().hash(), hashAfterFirstMove);
}

TEST(GameState, UndoSinglePlyGivesTurnBackToThatSide) {
    GameState g;
    ASSERT_TRUE(g.play({7, 7}, 0));   // black
    ASSERT_TRUE(g.play({7, 8}, 0));   // white
    ASSERT_TRUE(g.undo(1));
    EXPECT_EQ(g.sideToMove(), Stone::White);
}

TEST(GameState, FailedUndoClearsLastUndone) {
    GameState g;
    ASSERT_TRUE(g.play({7, 7}, 0));
    ASSERT_TRUE(g.play({7, 8}, 0));
    ASSERT_TRUE(g.undo(1));
    ASSERT_EQ(g.lastUndone().size(), 1u);

    EXPECT_FALSE(g.undo(5));
    EXPECT_TRUE(g.lastUndone().empty());
}

TEST(GameState, UndoAfterGameOverFails) {
    GameState g;
    for (int c = 0; c < 4; ++c) {
        ASSERT_TRUE(g.play({0, c}, 0));
        ASSERT_TRUE(g.play({1, c}, 0));
    }
    ASSERT_TRUE(g.play({0, 4}, 0));
    EXPECT_FALSE(g.undo(1));
}

TEST(GameState, FinishSetsResultAndReason) {
    GameState g;
    g.finish(GameResult::WhiteWin, ResultReason::Resign);
    EXPECT_EQ(g.result(), GameResult::WhiteWin);
    EXPECT_EQ(g.reason(), ResultReason::Resign);
    EXPECT_FALSE(g.play({0, 0}, 0));
}

TEST(GameState, FinishDoesNotOverrideExistingResult) {
    GameState g;
    for (int c = 0; c < 4; ++c) {
        ASSERT_TRUE(g.play({0, c}, 0));
        ASSERT_TRUE(g.play({1, c}, 0));
    }
    ASSERT_TRUE(g.play({0, 4}, 0));   // black wins by five
    ASSERT_EQ(g.result(), GameResult::BlackWin);

    g.finish(GameResult::WhiteWin, ResultReason::Disconnect);

    EXPECT_EQ(g.result(), GameResult::BlackWin);
    EXPECT_EQ(g.reason(), ResultReason::FiveInRow);
}
