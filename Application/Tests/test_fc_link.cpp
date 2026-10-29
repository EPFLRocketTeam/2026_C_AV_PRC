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
