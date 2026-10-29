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
