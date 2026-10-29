// Host tests for the pure policies in prc_commands.hpp. The handlers
// themselves (prc_commands.cpp) need the HAL and are not built here.

#include <gtest/gtest.h>

#include "Application/FlightControl/prc_commands.hpp"

using prc::BoardRole;
using prc::EngineState;
using prc::State;

namespace {

constexpr State kDprStates[] = {
    State::MANUAL, State::PRESSURIZE_ON, State::REGULATE, State::PRESSURIZE_OFF,
    State::DEPRESSURIZE_ON, State::DEPRESSURIZE_OFF, State::ABORT_ON_GROUND, State::ABORT_IN_FLIGHT,
};

}  // namespace

TEST(ConfigCommitAllowed, DprBoardsOnlyInManual) {
  for (BoardRole role : {BoardRole::DprLox, BoardRole::DprEth}) {
    for (State state : kDprStates) {
      // the engine state must not matter on a DPR board
      EXPECT_EQ(prc_commands::ConfigCommitAllowed(role, state, EngineState::Burn), state == State::MANUAL)
          << int(role) << " " << int(state);
    }
  }
}

TEST(ConfigCommitAllowed, EngineBoardOnlyInIdle) {
  const int last = static_cast<int>(EngineState::AbortInFlightEthanol);
  for (int i = 0; i <= last; i++) {
    const auto engine = static_cast<EngineState>(i);
    // the DPR state store is meaningless on the engine board
    EXPECT_EQ(prc_commands::ConfigCommitAllowed(BoardRole::EngineBay, State::MANUAL, engine),
              engine == EngineState::Idle) << i;
  }
}

TEST(ConfigCommitAllowed, NeverBeforeTheRoleIsLatched) {
  EXPECT_FALSE(prc_commands::ConfigCommitAllowed(BoardRole::Unknown, State::MANUAL, EngineState::Idle));
}

// system dfu and system reboot: the same window as config commit, written
// out as its own truth table so a change to one cannot loosen the other.
TEST(ResetAllowed, DprBoardsOnlyInManual) {
  for (BoardRole role : {BoardRole::DprLox, BoardRole::DprEth}) {
    for (State state : kDprStates) {
      EXPECT_EQ(prc_commands::ResetAllowed(role, state, EngineState::Idle), state == State::MANUAL)
          << int(role) << " " << int(state);
    }
  }
}

TEST(ResetAllowed, EngineBoardOnlyInIdle) {
  const int last = static_cast<int>(EngineState::AbortInFlightEthanol);
  for (int i = 0; i <= last; i++) {
    const auto engine = static_cast<EngineState>(i);
    EXPECT_EQ(prc_commands::ResetAllowed(BoardRole::EngineBay, State::MANUAL, engine),
              engine == EngineState::Idle) << i;
  }
}

TEST(ResetAllowed, RefusedBeforeTheRoleIsLatched) {
  EXPECT_FALSE(prc_commands::ResetAllowed(BoardRole::Unknown, State::MANUAL, EngineState::Idle));
}

using Command = prc_commands::ConfirmWindow::Command;

TEST(ConfirmWindow, ConfirmActsOnWhatWasArmedWithinTheWindow) {
  prc_commands::ConfirmWindow w;
  w.Arm(Command::kDfu, 1000);
  EXPECT_EQ(w.Confirm(1000 + 9999), Command::kDfu);
  EXPECT_EQ(w.Confirm(1000 + 9999), Command::kNone);  // consumed
  w.Arm(Command::kReboot, 5000);
  EXPECT_EQ(w.Confirm(5001), Command::kReboot);
}

TEST(ConfirmWindow, ConfirmWithNothingArmedDoesNothing) {
  prc_commands::ConfirmWindow w;
  EXPECT_EQ(w.Confirm(0), Command::kNone);
}

TEST(ConfirmWindow, WindowOfTenSecondsRunsOut) {
  prc_commands::ConfirmWindow w;
  w.Arm(Command::kDfu, 0);
  EXPECT_EQ(w.Confirm(10000), Command::kNone);
  EXPECT_EQ(w.Confirm(10001), Command::kNone);
}

// A reflex double entry only re-arms; nothing happens without system confirm.
TEST(ConfirmWindow, RepeatingTheCommandOnlyRearms) {
  prc_commands::ConfirmWindow w;
  w.Arm(Command::kDfu, 0);
  w.Arm(Command::kDfu, 9000);
  EXPECT_EQ(w.Confirm(18000), Command::kDfu);
}

TEST(ConfirmWindow, LastArmedCommandWins) {
  prc_commands::ConfirmWindow w;
  w.Arm(Command::kDfu, 0);
  w.Arm(Command::kReboot, 100);
  EXPECT_EQ(w.Confirm(200), Command::kReboot);
}

TEST(ConfirmWindow, DisarmCancelsTheWindow) {
  prc_commands::ConfirmWindow w;
  w.Arm(Command::kDfu, 0);
  w.Disarm();
  EXPECT_EQ(w.Confirm(1), Command::kNone);
}

TEST(ConfirmWindow, TickWrapAroundIsHandled) {
  prc_commands::ConfirmWindow w;
  w.Arm(Command::kDfu, 0xFFFFF000u);
  EXPECT_EQ(w.Confirm(0x00000500u), Command::kDfu);
}
