//    FILE: prc_fsm_c_api.h
// PURPOSE: C-callable entry points for the PRC Dynamic Pressure Regulator
//          FSM (prc_state.h/.cpp), for use from Core/Src/main.c — same
//          pattern as Drivers/FDC1004/FDC1004_manual_test.h: main.c is a
//          plain C file and can't include prc_state.h directly (classes,
//          namespaces, <string>).

#ifndef PRC_FSM_C_API_H
#define PRC_FSM_C_API_H

#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

// Call once at boot, after MX_GPIO_Init(). Also calls Valve_InitAll()
// internally, so callers don't need to call that separately. Latches this
// board's role by reading the ENG_SETUP/ETH_SETUP/LOX_SETUP straps -- see
// Drivers/PrcBoardId/PrcBoardId.hpp.
void Prc_Fsm_Init(void);

// Call once per main-loop iteration. Refreshes this tick's Event flags from
// sensor/uplink-command stores, advances the FSM, applies valve actions for
// the resulting state, and republishes the new state.
void Prc_Fsm_Tick(void);

#ifdef __cplusplus
}
#endif

#endif // PRC_FSM_C_API_H
