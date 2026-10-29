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
  int bad = 0;
  int unknown = 0;
};
Calls g_calls;
std::string g_sink_text;
int g_init_calls = 0;
bool g_init_binds = true;

void Sink(const char* text) { g_sink_text += text; }

prc_cli::driver FakeDriver() {
  prc_cli::driver d{};
  d.valve_vent  = [](void*, bool v) { g_calls.vent.push_back(v); };
  d.valve_ball  = [](void*, float v) { g_calls.ball.push_back(v); };
  d.state_print = [](void*) { g_calls.state++; };
  d.fsm_abort   = [](void*) { g_calls.abort++; };
  d.on_bad_argument = [](void*) { g_calls.bad++; };
  d.on_unknown_command = [](void*) { g_calls.unknown++; };
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

// The firmware binds the real handlers here (prc_commands.cpp). The tests
// bind theirs through prc_shell::Configure in SetUp; this stand in counts the
// lazy call Prc_Shell_Tick makes when no driver is bound, and binds the fake.
extern "C" void Prc_Shell_Init(void) {
  g_init_calls++;
  if (g_init_binds) prc_shell::Configure(&g_driver, Sink);
}

namespace {

// Drain the ring and return the producer index, so a test can place bytes at
// a known position.
uint16_t DrainedHead() {
  Prc_Shell_Tick();
  return prc_shell::RingHead();
}

}  // namespace

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

TEST_F(PrcShell, BareCrEndsALine) {
  Packet("valve vent on\rvalve ball 3\r");
  Prc_Shell_Tick();
  ASSERT_EQ(g_calls.vent.size(), 1u);
  EXPECT_TRUE(g_calls.vent[0]);
  ASSERT_EQ(g_calls.ball.size(), 1u);
  EXPECT_FLOAT_EQ(g_calls.ball[0], 3.0f);
}

TEST_F(PrcShell, LfRightAfterCrIsSwallowedEvenAcrossPackets) {
  Packet("state\r");
  Prc_Shell_Tick();
  Packet("\nstate\n\n");  // LF of the CRLF, then a line, then an empty line
  Prc_Shell_Tick();
  EXPECT_EQ(g_calls.state, 2);
  EXPECT_TRUE(g_sink_text.empty());
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

TEST_F(PrcShell, LineAcrossTheRingWrapPointRuns) {
  // Fill with empty lines until the producer index is 3 short of the end,
  // so "state\n" lands in slots 253, 254, 255, 0, 1, 2.
  const uint16_t head = DrainedHead();
  const uint16_t filler = static_cast<uint16_t>(prc_shell::kRingSize - 3 - head) % prc_shell::kRingSize;
  if (filler > 0) Packet(std::string(filler, '\n'));
  ASSERT_EQ(DrainedHead(), prc_shell::kRingSize - 3);

  Packet("state\n");
  EXPECT_EQ(prc_shell::RingHead(), 3);
  Prc_Shell_Tick();
  EXPECT_EQ(g_calls.state, 1);
  EXPECT_TRUE(g_sink_text.empty());
}

TEST_F(PrcShell, PacketOfExactly255BytesFitsAnEmptyRing) {
  DrainedHead();
  std::string packet = "state\n";
  packet += std::string(prc_shell::kRingSize - 1 - packet.size(), '\n');
  ASSERT_EQ(packet.size(), 255u);
  Packet(packet);
  Prc_Shell_Tick();
  EXPECT_EQ(g_calls.state, 1);
  EXPECT_TRUE(g_sink_text.empty());

  // 256 bytes never fit (one slot stays empty): dropped, and the next line
  // is dropped with it because the marker cannot tell where the hole ends.
  Packet(std::string(256, '\n'));
  Packet("state\n");
  Prc_Shell_Tick();
  EXPECT_EQ(g_calls.state, 1);
  EXPECT_NE(g_sink_text.find("input overflow"), std::string::npos);
}

TEST_F(PrcShell, SecondDroppedPacketWhileTheMarkerIsOwedStillDropsOneLine) {
  DrainedHead();
  Packet("valve ba");
  Packet(std::string(250, 'x'));  // dropped, marker owed
  Packet(std::string(250, 'y'));  // dropped again, marker still owed
  Prc_Shell_Tick();
  Packet("ll 7\n");
  Prc_Shell_Tick();
  EXPECT_TRUE(g_calls.ball.empty());
  EXPECT_EQ(g_sink_text, "[SHELL] input overflow, line dropped\r\n");

  Packet("valve ball 7\n");
  Prc_Shell_Tick();
  ASSERT_EQ(g_calls.ball.size(), 1u);
}

TEST_F(PrcShell, NulByteFromTheHostDropsTheLine) {
  Packet(std::string("sta\0te\n", 7));
  Prc_Shell_Tick();
  EXPECT_EQ(g_calls.state, 0);
  EXPECT_NE(g_sink_text.find("input overflow"), std::string::npos);
}

TEST_F(PrcShell, DamagedAndOverlongLineReportsTheDamage) {
  DrainedHead();
  Packet(std::string(200, 'z'));
  Packet(std::string(250, 'x'));  // dropped while 200 bytes wait
  Prc_Shell_Tick();
  Packet("\n");
  Prc_Shell_Tick();
  EXPECT_EQ(g_sink_text, "[SHELL] input overflow, line dropped\r\n");
}

TEST_F(PrcShell, LineOf129BytesIsDropped) {
  std::string line = "valve ball 5";
  line += std::string(prc_shell::kLineMax + 1 - line.size(), '0');
  ASSERT_EQ(line.size(), prc_shell::kLineMax + 1);
  Packet(line + "\n");
  Prc_Shell_Tick();
  EXPECT_TRUE(g_calls.ball.empty());
  EXPECT_NE(g_sink_text.find("longer than 128 bytes"), std::string::npos);
}

TEST_F(PrcShell, EmptyOrNullPacketIsIgnored) {
  Prc_Shell_ProcessRxBytes(nullptr, 5);
  const uint8_t byte = 's';
  Prc_Shell_ProcessRxBytes(&byte, 0);
  const uint16_t before = prc_shell::RingHead();
  Prc_Shell_Tick();
  EXPECT_EQ(prc_shell::RingHead(), before);
  EXPECT_TRUE(g_sink_text.empty());
}

TEST_F(PrcShell, BadBoolArgumentRunsNothingAndCallsOnBadArgument) {
  Packet("valve vent maybe\nvalve vent opne\nvalve vent close\n");
  Prc_Shell_Tick();
  EXPECT_EQ(g_calls.bad, 2);
  EXPECT_EQ(g_calls.vent, (std::vector<bool>{false}));
}

TEST_F(PrcShell, GarbageBeforeACommandRunsNothing) {
  Packet("xfsm abort\nfsm abort\n");
  Prc_Shell_Tick();
  EXPECT_EQ(g_calls.abort, 1);
  EXPECT_EQ(g_calls.unknown, 1);
}

TEST_F(PrcShell, UnknownLinesGetOnUnknownCommandAndEmptyLinesNothing) {
  Packet(" state\nstat\nvalve\nzzz\n\n   \nstate\n");
  Prc_Shell_Tick();
  EXPECT_EQ(g_calls.unknown, 4);
  EXPECT_EQ(g_calls.state, 1);
}

TEST_F(PrcShell, TickBindsTheHandlersOnceWhenNoneAreBound) {
  prc_shell::Configure(nullptr, nullptr);
  g_init_calls = 0;
  Prc_Shell_Tick();
  EXPECT_EQ(g_init_calls, 1);
  Prc_Shell_Tick();
  Packet("state\n");
  Prc_Shell_Tick();
  EXPECT_EQ(g_init_calls, 1);
  EXPECT_EQ(g_calls.state, 1);

  // Init again (idempotent): same binding, the shell keeps working.
  Prc_Shell_Init();
  Packet("state\n");
  Prc_Shell_Tick();
  EXPECT_EQ(g_calls.state, 2);
}

TEST_F(PrcShell, NoDriverBoundRunsNothing) {
  g_init_binds = false;
  prc_shell::Configure(nullptr, Sink);
  Packet("state\n");
  Prc_Shell_Tick();
  EXPECT_EQ(g_calls.state, 0);
  g_init_binds = true;
}

TEST_F(PrcShell, NoSinkBoundDropsTheShellMessageQuietly) {
  prc_shell::Configure(&g_driver, nullptr);
  Packet(std::string(200, 'z') + "\nstate\n");
  Prc_Shell_Tick();
  EXPECT_EQ(g_calls.state, 1);
  EXPECT_TRUE(g_sink_text.empty());
}

TEST_F(PrcShell, UsbLogAlreadyOnIsLeftOn) {
  app_printf_use_usb(true);
  prc_shell::Configure(&g_driver, Sink);
  Packet("state\n");
  Prc_Shell_Tick();
  EXPECT_TRUE(app_printf_is_usb_enabled());
  EXPECT_EQ(g_calls.state, 1);
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
