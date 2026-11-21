#include <gtest/gtest.h>

#include "Application/Data/data.hpp"

using prc::BoardRole;
using prc::LinkStore;
using prc::link_board_for_role;
using prc_intranet::payload::heartbeat;
using namespace prc_intranet::constants;

namespace {

heartbeat Beat(uint32_t uptime_ms, uint16_t counter, uint8_t alive_mask = 0) {
    heartbeat hb{};
    hb.uptime_ms  = uptime_ms;
    hb.counter    = counter;
    hb.alive_mask = alive_mask;
    return hb;
}

LinkStore Started(BoardRole role) {
    LinkStore links;
    links.start(link_board_for_role(role));
    return links;
}

} // namespace

// --- Role -> board index -----------------------------------------------------

TEST(LinkStore, RoleMapsToMatrixIndex) {
    EXPECT_EQ(link_board_for_role(BoardRole::EngineBay), LINK_BIT_ENGINE);
    EXPECT_EQ(link_board_for_role(BoardRole::DprEth),    LINK_BIT_ETH);
    EXPECT_EQ(link_board_for_role(BoardRole::DprLox),    LINK_BIT_LOX);
    EXPECT_EQ(link_board_for_role(BoardRole::Unknown),   prc::LINK_NO_BOARD);
}

// --- Unknown role: nothing tracked -------------------------------------------

TEST(LinkStore, UnknownRoleTracksNothing) {
    LinkStore links = Started(BoardRole::Unknown);
    EXPECT_FALSE(links.started());
    links.on_heartbeat(LINK_BIT_FC, Beat(1000, 0), 1000);
    EXPECT_FALSE(links.alive(LINK_BIT_FC, 1000));
    EXPECT_EQ(links.own_row(1000), 0u);
    EXPECT_EQ(links.take_report(5000), 0u);
    EXPECT_EQ(links.take_reboot_report(), 0u);
}

TEST(LinkStore, StartIsLatchedOnce) {
    LinkStore links = Started(BoardRole::DprEth);
    links.start(LINK_BIT_LOX);                     // ignored: already started
    EXPECT_EQ(links.own_row(0), 0x4u);             // still ETH
}

// --- Tracking the 3 peers ----------------------------------------------------

TEST(LinkStore, EthHearsFcAndBothPrc) {
    LinkStore links = Started(BoardRole::DprEth);
    EXPECT_EQ(links.own_row(0), 0x4u);             // only itself
    links.on_heartbeat(LINK_BIT_FC,     Beat(5000, 0), 100);
    links.on_heartbeat(LINK_BIT_ENGINE, Beat(5000, 0), 110);
    links.on_heartbeat(LINK_BIT_LOX,    Beat(5000, 0), 120);
    EXPECT_EQ(links.own_row(200), 0xFu);
    EXPECT_TRUE(links.alive(LINK_BIT_LOX, 200));
}

TEST(LinkStore, OwnBeatIsIgnored) {
    LinkStore links = Started(BoardRole::DprLox);
    links.on_heartbeat(LINK_BIT_LOX, Beat(5000, 0), 100);
    EXPECT_EQ(links.peer(LINK_BIT_LOX).received, 0u);
    EXPECT_EQ(links.own_row(100), 0x8u);
}

TEST(LinkStore, PeerTimesOutAfter500ms) {
    LinkStore links = Started(BoardRole::EngineBay);
    links.on_heartbeat(LINK_BIT_ETH, Beat(5000, 0), 1000);
    EXPECT_EQ(links.own_row(1499), 0x6u);          // ETH + ENGINE
    EXPECT_EQ(links.own_row(1500), 0x2u);          // ETH timed out
}

TEST(LinkStore, CountsLostBeatsPerPeer) {
    LinkStore links = Started(BoardRole::DprLox);
    links.on_heartbeat(LINK_BIT_ENGINE, Beat(5000, 10), 0);
    links.on_heartbeat(LINK_BIT_ENGINE, Beat(5150, 13), 150);  // 11, 12 lost
    links.on_heartbeat(LINK_BIT_FC,     Beat(5000, 7), 0);
    links.on_heartbeat(LINK_BIT_FC,     Beat(5050, 8), 50);
    EXPECT_EQ(links.peer(LINK_BIT_ENGINE).missed, 2u);
    EXPECT_EQ(links.peer(LINK_BIT_FC).missed, 0u);
}

TEST(LinkStore, ReportsAPeerReboot) {
    LinkStore links = Started(BoardRole::DprEth);
    links.on_heartbeat(LINK_BIT_FC, Beat(60000, 1200), 0);
    links.on_heartbeat(LINK_BIT_FC, Beat(3000, 0), 4000);      // uptime went back
    EXPECT_EQ(links.take_reboot_report(), 0x1u);
    EXPECT_EQ(links.take_reboot_report(), 0u);
    EXPECT_EQ(links.peer(LINK_BIT_FC).reboots, 1u);
}

// --- Console reports ---------------------------------------------------------

// On the board the first telemetry call comes about 12 s after reset (the
// boot countdown); the grace then runs 2 s from that call.
TEST(LinkStore, GraceFromFirstReportThenEdges) {
    LinkStore links = Started(BoardRole::DprLox);
    links.on_heartbeat(LINK_BIT_FC, Beat(12000, 0), 11990);
    EXPECT_EQ(links.take_report(12000), 0x1u);     // FC alive: edge
    links.on_heartbeat(LINK_BIT_FC, Beat(13990, 40), 13990);
    EXPECT_EQ(links.take_report(13999), 0u);       // inside the grace
    EXPECT_EQ(links.take_report(14000), 0x6u);     // ENGINE and ETH never heard: once
    EXPECT_EQ(links.take_report(14050), 0u);
    EXPECT_EQ(links.take_report(14500), 0x1u);     // FC dead
}

TEST(LinkStore, ReportNeverHasOwnBit) {
    LinkStore links = Started(BoardRole::EngineBay);
    EXPECT_EQ(links.take_report(0), 0u);
    EXPECT_EQ(links.take_report(2000), 0xDu);      // FC, ETH, LOX never heard
}
