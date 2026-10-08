#include <gtest/gtest.h>

#include "FakeTimeSource.h"
#include "ITimeSource.h"

TEST(FakeTimeSource, StartsAtGivenTimeAndStaysPut) {
    FakeTimeSource t(1000);
    EXPECT_EQ(t.nowMs(), 1000);
    EXPECT_EQ(t.nowMs(), 1000);
}

TEST(FakeTimeSource, AdvanceAndSet) {
    FakeTimeSource t;
    EXPECT_EQ(t.nowMs(), 0);
    t.advance(250);
    EXPECT_EQ(t.nowMs(), 250);
    t.set(5000);
    EXPECT_EQ(t.nowMs(), 5000);
}

TEST(FakeTimeSource, AutoAdvanceMovesAfterEachRead) {
    FakeTimeSource t(0, 10);
    EXPECT_EQ(t.nowMs(), 0);
    EXPECT_EQ(t.nowMs(), 10);
    EXPECT_EQ(t.nowMs(), 20);
    t.setAutoAdvance(0);
    EXPECT_EQ(t.nowMs(), 30);
    EXPECT_EQ(t.nowMs(), 30);
}

TEST(FakeTimeSource, CountsReads) {
    FakeTimeSource t;
    EXPECT_EQ(t.reads(), 0);
    t.nowMs();
    t.nowMs();
    EXPECT_EQ(t.reads(), 2);
}

TEST(FakeTimeSource, UsableThroughInterface) {
    FakeTimeSource fake(42);
    const core::ITimeSource& time = fake;
    EXPECT_EQ(time.nowMs(), 42);
}
