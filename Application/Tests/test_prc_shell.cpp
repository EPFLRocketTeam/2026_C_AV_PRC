// Host tests for the PRC USB shell (Application/FlightControl/prc_shell.cpp):
// the interrupt side ring buffer, the main loop line assembler and the
// dispatch into the generated prc_cli trie. The real handlers
// (prc_commands.cpp) need the HAL, so a fake driver records the calls and an
// injected sink captures the shell's own messages.

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "Application/FlightControl/prc_shell.hpp"
#include "Application/app_printf.h"

namespace {

struct Calls {
  std::vector<bool>  vent;
  std::vector<float> ball;
  int state = 0;
  int abort = 0;
};
Calls g_calls;
std::string g_sink_text;

void Sink(const char* text) { g_sink_text += text; }

prc_cli::driver FakeDriver() {
  prc_cli::driver d{};
  d.valve_vent  = [](void*, bool v) { g_calls.vent.push_back(v); };
  d.valve_ball  = [](void*, float v) { g_calls.ball.push_back(v); };
  d.state_print = [](void*) { g_calls.state++; };
  d.fsm_abort   = [](void*) { g_calls.abort++; };
  return d;
}

prc_cli::driver g_driver;

void Packet(const std::string& bytes) {
  Prc_Shell_ProcessRxBytes(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size());
}

class PrcShell : public ::testing::Test {
protected:
  void SetUp() override {
    g_calls = Calls();
    g_sink_text.clear();
    g_driver = FakeDriver();
    prc_shell::Configure(&g_driver, Sink);
  }
};

}  // namespace

// The firmware binds the real handlers here; the tests bind theirs through
// prc_shell::Configure before the first tick, so this is never reached.
extern "C" void Prc_Shell_Init(void) {}

TEST_F(PrcShell, LineSplitOverTwoPacketsWithATickBetweenSurvives) {
  Packet("valve ve");
  Prc_Shell_Tick();
  EXPECT_TRUE(g_calls.vent.empty());
  Packet("nt open\n");
  Prc_Shell_Tick();
  ASSERT_EQ(g_calls.vent.size(), 1u);
  EXPECT_TRUE(g_calls.vent[0]);
}

TEST_F(PrcShell, LineSplitOverTwoPacketsBeforeATickSurvives) {
  Packet("valve ball 4");
  Packet("2.5\n");
  Prc_Shell_Tick();
  ASSERT_EQ(g_calls.ball.size(), 1u);
  EXPECT_FLOAT_EQ(g_calls.ball[0], 42.5f);
}

TEST_F(PrcShell, CrLfEndingsAreAccepted) {
  Packet("valve vent on\r\nvalve ball 12.5\r\n");
  Prc_Shell_Tick();
  ASSERT_EQ(g_calls.vent.size(), 1u);
  EXPECT_TRUE(g_calls.vent[0]);
  ASSERT_EQ(g_calls.ball.size(), 1u);
  EXPECT_FLOAT_EQ(g_calls.ball[0], 12.5f);
}

TEST_F(PrcShell, TrailingSpacesAreTrimmedSoOpenStillMeansOpen) {
  Packet("valve vent open   \n");
  Prc_Shell_Tick();
  ASSERT_EQ(g_calls.vent.size(), 1u);
  EXPECT_TRUE(g_calls.vent[0]);
}

TEST_F(PrcShell, BoolArgumentParsesOpenAndOnAsTrue) {
  Packet("valve vent open\nvalve vent on\nvalve vent close\nvalve vent off\n");
  Prc_Shell_Tick();
  EXPECT_EQ(g_calls.vent, (std::vector<bool>{true, true, false, false}));
}

TEST_F(PrcShell, VoidCommandsFireAndSeveralLinesInOnePacketAllRun) {
  Packet("state\nfsm abort\n");
  Prc_Shell_Tick();
  EXPECT_EQ(g_calls.state, 1);
  EXPECT_EQ(g_calls.abort, 1);
}

TEST_F(PrcShell, OverlongLineIsDroppedNotTruncated) {
  // A truncated "valve ball 1000..." would still run; it must not.
  Packet("valve ball 1" + std::string(200, '0') + "\n");
  Prc_Shell_Tick();
  EXPECT_TRUE(g_calls.ball.empty());
  EXPECT_NE(g_sink_text.find("longer than 128 bytes"), std::string::npos);

  // and the assembler is clean again for the next line
  g_sink_text.clear();
  Packet("valve ball 7\n");
  Prc_Shell_Tick();
  ASSERT_EQ(g_calls.ball.size(), 1u);
  EXPECT_FLOAT_EQ(g_calls.ball[0], 7.0f);
  EXPECT_TRUE(g_sink_text.empty());
}

TEST_F(PrcShell, LineOfExactly128BytesIsAccepted) {
  std::string line = "valve ball 5";
  line += std::string(prc_shell::kLineMax - line.size(), ' ');
  ASSERT_EQ(line.size(), prc_shell::kLineMax);
  Packet(line + "\n");
  Prc_Shell_Tick();
  ASSERT_EQ(g_calls.ball.size(), 1u);
  EXPECT_FLOAT_EQ(g_calls.ball[0], 5.0f);
}

TEST_F(PrcShell, PacketThatOverflowsTheRingDropsItsWholeLine) {
  Packet("valve ba");
  Packet(std::string(250, 'x'));  // does not fit beside the 8 bytes queued
  Prc_Shell_Tick();
  Packet("ll 7\n");               // the tail of the damaged line
  Prc_Shell_Tick();
  EXPECT_TRUE(g_calls.ball.empty());
  EXPECT_NE(g_sink_text.find("input overflow"), std::string::npos);

  Packet("valve ball 7\n");
  Prc_Shell_Tick();
  ASSERT_EQ(g_calls.ball.size(), 1u);
  EXPECT_FLOAT_EQ(g_calls.ball[0], 7.0f);
}

TEST_F(PrcShell, EmptyLinesAndUnknownWordsDoNothing) {
  Packet("\r\n\n   \nzzz\n");
  Prc_Shell_Tick();
  EXPECT_TRUE(g_calls.vent.empty());
  EXPECT_EQ(g_calls.state, 0);
  EXPECT_TRUE(g_sink_text.empty());
}

TEST_F(PrcShell, FirstReceivedBytesTurnTheUsbLogOnOnce) {
  app_printf_use_usb(false);
  prc_shell::Configure(&g_driver, Sink);

  Prc_Shell_Tick();  // nothing received yet: the log stays off
  EXPECT_FALSE(app_printf_is_usb_enabled());

  Packet("state\n");
  Prc_Shell_Tick();
  EXPECT_TRUE(app_printf_is_usb_enabled());

  // turned off afterwards (e.g. "logs usb off"): the shell leaves it off
  app_printf_use_usb(false);
  Packet("state\n");
  Prc_Shell_Tick();
  EXPECT_FALSE(app_printf_is_usb_enabled());
  EXPECT_EQ(g_calls.state, 2);
}
