#include <gtest/gtest.h>

#include "Types.h"

using namespace core;

TEST(Types, OpponentSwapsColors) {
    EXPECT_EQ(opponent(Stone::Black), Stone::White);
    EXPECT_EQ(opponent(Stone::White), Stone::Black);
}
