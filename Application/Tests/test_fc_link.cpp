#include <gtest/gtest.h>

#include "Application/Data/data.hpp"

using prc::FcLinkStore;

TEST(FcLink, DeadBeforeAnyBeat) {
    FcLinkStore link;
    EXPECT_FALSE(link.alive(0));
    EXPECT_FALSE(link.alive(100));
}

TEST(FcLink, AliveAfterOneBeat) {
    FcLinkStore link;
    link.on_heartbeat(7, 1000);
    EXPECT_TRUE(link.alive(1000));
    EXPECT_TRUE(link.alive(1499));
    EXPECT_EQ(link.get().last_counter, 7u);
    EXPECT_EQ(link.get().last_rx_ms, 1000u);
}

TEST(FcLink, DeadAfter500ms) {
    FcLinkStore link;
    link.on_heartbeat(0, 1000);
    EXPECT_FALSE(link.alive(1500));
    link.on_heartbeat(1, 1550);
    EXPECT_TRUE(link.alive(1600));
}

TEST(FcLink, SurvivesTickWrap) {
    FcLinkStore link;
    link.on_heartbeat(3, 0xFFFFFF00u);
    EXPECT_TRUE(link.alive(0x00000010u));
    EXPECT_FALSE(link.alive(0x00000200u));
}

TEST(FcLink, BootGraceReportsNeverSeenOnce) {
    FcLinkStore link;
    EXPECT_FALSE(link.take_report(100, link.alive(100)));
    EXPECT_FALSE(link.take_report(1999, link.alive(1999)));
    EXPECT_TRUE(link.take_report(2000, link.alive(2000)));   // one "dead"
    EXPECT_FALSE(link.take_report(2050, link.alive(2050)));
}

TEST(FcLink, EdgesOnly) {
    FcLinkStore link;
    link.on_heartbeat(0, 1000);
    EXPECT_TRUE(link.take_report(1000, link.alive(1000)));    // alive edge
    EXPECT_FALSE(link.take_report(1400, link.alive(1400)));
    EXPECT_TRUE(link.take_report(1500, link.alive(1500)));    // dead edge
    EXPECT_FALSE(link.take_report(2000, link.alive(2000)));   // seen before: grace adds nothing
    link.on_heartbeat(41, 2100);
    EXPECT_TRUE(link.take_report(2100, link.alive(2100)));    // back
    EXPECT_EQ(link.get().last_counter, 41u);
}
