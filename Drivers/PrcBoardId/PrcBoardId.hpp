//    FILE: PrcBoardId.hpp
// PURPOSE: Runtime board-identity detection — this firmware image runs on
//          3 physical boards (2026_C_SE_FLIGHT-PARAMETERS: PRC-L "DPR LOx",
//          PRC-E "DPR Eth", PRC-P "Engine Bay"), and which one applies is a
//          hardware fact about the board it's flashed to, not a build-time
//          choice.
//
// Scheme: 3 dedicated strap pins, one per board (ENG_SETUP/ETH_SETUP/
// LOX_SETUP -- PA1/PA2/PA3, see Core/Inc/main.h), each board's harness
// driving exactly one of the three high. No parameters -- these are fixed
// hardware pins, not a generic port/pin the caller supplies.

#ifndef DRIVERS_PRC_BOARD_ID_H
#define DRIVERS_PRC_BOARD_ID_H

#include "Application/Data/data.hpp" // prc::BoardRole

// Reads ENG_SETUP/ETH_SETUP/LOX_SETUP and returns the detected board role.
// Returns BoardRole::Unknown if none or more than one reads high (open
// strap / wiring fault) -- ambiguous readings are never resolved to a
// guess, since misidentifying LOX vs Eth has real consequences.
prc::BoardRole Prc_DetectBoardRole();

#endif // DRIVERS_PRC_BOARD_ID_H
