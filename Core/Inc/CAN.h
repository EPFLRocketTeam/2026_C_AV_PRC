#ifndef CAN_H
#define CAN_H

#include <stdint.h>

/* ============================================================
 * 1. CAN ID Structure (Standard 11-bit) - Main Bus
 *    Bits [10:8] = Priority / Type
 *    Bits [7:4]  = Target Node ID
 *    Bits [3:0]  = Message Index
 * ============================================================ */

#define CAN_ID(prio, node, idx) \
    ((uint16_t)((((prio) & 0x7) << 8) | (((node) & 0xF) << 4) | ((idx) & 0xF)))

/* --- 1.1 Priority / Type (Bits 10-8) --- */
#define CAN_PRIO_EMERGENCY  0x0 /* Abort, Passivate. Preempts all other traffic. */
#define CAN_PRIO_HIGH_CMD   0x1 /* Time-critical commands (Ignite, Pressurize). */
#define CAN_PRIO_LOW_CMD    0x2 /* State changes, manual valve controls, resets. */
#define CAN_PRIO_TEL_HIGH   0x3 /* High-rate telemetry (Pressures, FSM State). */
#define CAN_PRIO_TEL_LOW    0x4 /* Low-rate telemetry (Temps, INA228/TMP1075/LMT85). */
#define CAN_PRIO_SYSTEM     0x5 /* Timestamp sync, Heartbeats. */

/* --- 1.2 Target Node ID (Bits 7-4) --- */
#define CAN_NODE_FC         0x0 /* Flight Computer (FC) / Broadcast */
#define CAN_NODE_PRC_P      0x1 /* Propulsion Computer ENGINE */
#define CAN_NODE_DPR_ETH    0x2 /* PRC/DPR Ethanol */
#define CAN_NODE_DPR_LOX    0x3 /* PRC/DPR LOX */

/* ============================================================
 * 2. CAN Message Dictionary - Main Bus
 * ============================================================ */

/* --- 2.1 Broadcasts (FC -> All) --- */
#define CANID_BROADCAST_ABORT      CAN_ID(CAN_PRIO_EMERGENCY, CAN_NODE_FC, 0x0) /* 0x000 */
#define CANID_TIME_SYNC            CAN_ID(CAN_PRIO_SYSTEM,    CAN_NODE_FC, 0x0) /* 0x500 */

/* --- 2.2 Propulsion Computer (PRC - Node 1) --- */
/* FC -> PRC (Commands) */
#define CANID_PRC_PASSIVATE        CAN_ID(CAN_PRIO_EMERGENCY, CAN_NODE_PRC_P, 0x0) /* 0x010 */
#define CANID_PRC_IGNITE           CAN_ID(CAN_PRIO_HIGH_CMD,  CAN_NODE_PRC_P, 0x0) /* 0x110 */
#define CANID_PRC_CLEAR_TO_IGNITE  CAN_ID(CAN_PRIO_LOW_CMD,   CAN_NODE_PRC_P, 0x0) /* 0x210 */
#define CANID_PRC_CMD_VALVES       CAN_ID(CAN_PRIO_LOW_CMD,   CAN_NODE_PRC_P, 0x1) /* 0x211 */
#define CANID_PRC_RESET            CAN_ID(CAN_PRIO_LOW_CMD,   CAN_NODE_PRC_P, 0x2) /* 0x212 */

/* PRC -> FC (Telemetry) */
#define CANID_PRC_STATE            CAN_ID(CAN_PRIO_TEL_HIGH,  CAN_NODE_PRC_P, 0x0) /* 0x310 */
#define CANID_PRC_P_CHAMBER        CAN_ID(CAN_PRIO_TEL_HIGH,  CAN_NODE_PRC_P, 0x1) /* 0x311 */
#define CANID_PRC_P_INJECTOR       CAN_ID(CAN_PRIO_TEL_HIGH,  CAN_NODE_PRC_P, 0x2) /* 0x312 */
#define CANID_PRC_T_CHAMBER        CAN_ID(CAN_PRIO_TEL_LOW,   CAN_NODE_PRC_P, 0x0) /* 0x410 */
#define CANID_PRC_T_INJECTOR       CAN_ID(CAN_PRIO_TEL_LOW,   CAN_NODE_PRC_P, 0x1) /* 0x411 */

/* PRC_CMD_VALVES / DPR_{ETH,LOX}_CMD_VALVES payload -- CORRECTED per
 * CAN.md's own sub-table + team decision: this is a SINGLE valve
 * (ID + open/closed state) per message, never a simultaneous multi-valve
 * bitmask (the doc's top-level "[Bitmask][Magic]" header and its own
 * worked example ("0x211A31" = valve ID 0xA, described as "solenoid 3"
 * even though the sub-table maps 0xA -> Sol 1) both contradicted the
 * sub-table and each other -- resolved 2026-07-20: always single
 * valve-ID + state, "we will never want to open valves simultaneously". */
#define VALVE_SOL1          0xA
#define VALVE_SOL2          0xB
#define VALVE_SOL3          0xC
#define VALVE_SOL4          0xD

/* DPR (DPR_ETH/DPR_LOX) CMD_VALVES valve IDs -- CAN.md names these
 * DN/PX/VX (see its "DN, PX, VX control" description for
 * DPR_{ETH,LOX}_CMD_VALVES) but never assigns byte values for them,
 * unlike the engine PRC's Sol1-4. PLACEHOLDERS -- to be defined for real
 * before use; chosen clear of VALVE_SOL1-4's range (0xA-0xD) for now. */
#define VALVE_DN            0xE0 /* TODO: placeholder, not yet assigned */
#define VALVE_PX            0xE1 /* TODO: placeholder, not yet assigned */
#define VALVE_VX            0xE2 /* TODO: placeholder, not yet assigned */

#define VALVE_STATE_OPEN    0x1
#define VALVE_STATE_CLOSED  0x0

/* --- 2.3 DPR Ethanol (DPR_ETH - Node 2) --- */
/* FC -> DPR_ETH (Commands) */
#define CANID_DPR_ETH_ABORT        CAN_ID(CAN_PRIO_EMERGENCY, CAN_NODE_DPR_ETH, 0x0) /* 0x020 */
#define CANID_DPR_ETH_PASSIVATE    CAN_ID(CAN_PRIO_EMERGENCY, CAN_NODE_DPR_ETH, 0x1) /* 0x021 */
#define CANID_DPR_ETH_PRESSURIZE   CAN_ID(CAN_PRIO_HIGH_CMD,  CAN_NODE_DPR_ETH, 0x0) /* 0x120 */
#define CANID_DPR_ETH_CMD_VALVES   CAN_ID(CAN_PRIO_LOW_CMD,   CAN_NODE_DPR_ETH, 0x0) /* 0x220 */
#define CANID_DPR_ETH_RESET        CAN_ID(CAN_PRIO_LOW_CMD,   CAN_NODE_DPR_ETH, 0x1) /* 0x221 */

/* DPR_ETH -> FC (Telemetry) */
#define CANID_DPR_ETH_STATE        CAN_ID(CAN_PRIO_TEL_HIGH,  CAN_NODE_DPR_ETH, 0x0) /* 0x320 */
#define CANID_DPR_ETH_PRESSURES    CAN_ID(CAN_PRIO_TEL_HIGH,  CAN_NODE_DPR_ETH, 0x1) /* 0x321 */
#define CANID_DPR_ETH_TEMPS_1      CAN_ID(CAN_PRIO_TEL_LOW,   CAN_NODE_DPR_ETH, 0x0) /* 0x420 */
#define CANID_DPR_ETH_TEMPS_2      CAN_ID(CAN_PRIO_TEL_LOW,   CAN_NODE_DPR_ETH, 0x1) /* 0x421 */

/* --- 2.4 DPR LOX (DPR_LOX - Node 3) --- */
/* FC -> DPR_LOX (Commands) */
#define CANID_DPR_LOX_ABORT        CAN_ID(CAN_PRIO_EMERGENCY, CAN_NODE_DPR_LOX, 0x0) /* 0x030 */
#define CANID_DPR_LOX_PASSIVATE    CAN_ID(CAN_PRIO_EMERGENCY, CAN_NODE_DPR_LOX, 0x1) /* 0x031 */
#define CANID_DPR_LOX_PRESSURIZE   CAN_ID(CAN_PRIO_HIGH_CMD,  CAN_NODE_DPR_LOX, 0x0) /* 0x130 */
#define CANID_DPR_LOX_CMD_VALVES   CAN_ID(CAN_PRIO_LOW_CMD,   CAN_NODE_DPR_LOX, 0x0) /* 0x230 */
#define CANID_DPR_LOX_RESET        CAN_ID(CAN_PRIO_LOW_CMD,   CAN_NODE_DPR_LOX, 0x1) /* 0x231 */

/* DPR_LOX -> FC (Telemetry) */
#define CANID_DPR_LOX_STATE        CAN_ID(CAN_PRIO_TEL_HIGH,  CAN_NODE_DPR_LOX, 0x0) /* 0x330 */
#define CANID_DPR_LOX_PRESSURES    CAN_ID(CAN_PRIO_TEL_HIGH,  CAN_NODE_DPR_LOX, 0x1) /* 0x331 */
#define CANID_DPR_LOX_TEMPS_1      CAN_ID(CAN_PRIO_TEL_LOW,   CAN_NODE_DPR_LOX, 0x0) /* 0x430 */
#define CANID_DPR_LOX_TEMPS_2      CAN_ID(CAN_PRIO_TEL_LOW,   CAN_NODE_DPR_LOX, 0x1) /* 0x431 */

/* --- Common command values --- */
#define CMD_ON              0x64
#define CMD_OFF             0x0D
#define RESET_MAGIC         0xDEADBEEF

/* ============================================================
 * 2.5 Safety Keys (EMERGENCY payload validation constants)
 * All are placeholders except BROADCAST_ABORT and PRC_PASSIVATE,
 * which were given in the doc. Confirm/replace before flight -
 * these must be unique per command and agreed with the whole team.
 * ============================================================ */
#define SAFETY_KEY_BROADCAST_ABORT     0xABBABBFF /* from CAN.md */
#define SAFETY_KEY_PRC_PASSIVATE       0x42424242 /* from CAN.md */

#define SAFETY_KEY_DPR_ETH_ABORT       0x2A2A2A2A
#define SAFETY_KEY_DPR_ETH_PASSIVATE   0x2B2B2B2B
#define SAFETY_KEY_DPR_LOX_ABORT       0x3A3A3A3A
#define SAFETY_KEY_DPR_LOX_PASSIVATE   0x3B3B3B3B

/* ============================================================
 * 3. Cameras - Separate Physical CAN Bus
 *
 * TODO: CAN.md section 3.1's node table (0x0=AERO_BOT, 0x1=AERO_TOP,
 * 0x2=SEPMECH, 0x3=FC) does not match the literal IDs below - all
 * three cameras decode to node=5 with only the priority nibble
 * changing (0x0N0/0x1N0/0x2N0), and the section header itself says
 * "Nodes 5, 6, 7" (three distinct nodes), a third description that
 * doesn't match either of the other two. Using the literal IDs as
 * given since they're unambiguous; the node table/header need to be
 * reconciled separately. CAN.md also flags its own open question on
 * whether FC should be node 0x0 or 0x3 on this bus ("since it is the
 * master") -- unresolved.
 * ============================================================ */

/* FC -> CAM (Commands) */
#define CANID_CAM_AERO_BOT_REC     0x050
#define CANID_CAM_AERO_BOT_PWR     0x051
#define CANID_CAM_AERO_TOP_REC     0x150
#define CANID_CAM_AERO_TOP_PWR     0x151
#define CANID_CAM_SEPMECH_REC      0x250
#define CANID_CAM_SEPMECH_PWR      0x251

/* CAM -> FC (Telemetry) */
#define CANID_CAM_SEPMECH_STATUS   0x350

/* ============================================================
 * Payload Structs
 * ============================================================ */
#pragma pack(push, 1)

typedef struct {
    uint32_t safety_key;
} payload_safety_key_t;

typedef struct {
    uint32_t timestamp_ms;
} payload_timestamp_t;

typedef struct {
    uint8_t state; /* CMD_ON / CMD_OFF */
} payload_on_off_t;

/* Single valve-ID + state -- see the CMD_VALVES correction note above. */
typedef struct {
    uint8_t valve_id; /* VALVE_SOL1..4 (PRC) or VALVE_DN/PX/VX (DPR, placeholders) */
    uint8_t state;    /* VALVE_STATE_OPEN / VALVE_STATE_CLOSED */
} payload_cmd_valves_t;

typedef struct {
    uint32_t magic; /* RESET_MAGIC */
} payload_reset_t;

typedef struct {
    uint8_t fsm_state;
    uint8_t valve_mask;
} payload_prc_state_t;

typedef struct {
    float p_oin;
    float p_ein;
} payload_prc_p_chamber_t;

typedef struct {
    float p_ccc;
    float p_cig;
} payload_prc_p_injector_t;

typedef struct {
    float t_oin;
    float t_ein;
} payload_prc_t_chamber_t;

typedef struct {
    float t_ccc;
    uint8_t reserved[4];
} payload_prc_t_injector_t;

/* fsm_state added -- CAN.md's DPR_STATE payload was missing it (unlike
 * PRC_STATE); DLC for CANID_DPR_{ETH,LOX}_STATE is now 2 bytes, not 1. */
typedef struct {
    uint8_t fsm_state;
    uint8_t valve_mask;
} payload_dpr_state_t;

typedef struct {
    float p_xta;
    float p_nco;
} payload_dpr_pressures_t;

typedef struct {
    float t_xta;
    float t_nco;
} payload_dpr_temps_1_t;

/* DPR_ETH_TEMPS_2: [T_COPV_EXT] [T_FLS_90] per CAN.md -- distinct from LOX,
 * see payload_dpr_lox_temps_2_t. */
typedef struct {
    float t_copv_ext;
    float t_fls_90;
} payload_dpr_eth_temps_2_t;

/* DPR_LOX_TEMPS_2: [T_FLS_80] [T_FLS_50] per CAN.md -- no COPV-external
 * field, unlike DPR_ETH's TEMPS_2 -- CANNOT share a struct with it. */
typedef struct {
    float t_fls_80;
    float t_fls_50;
} payload_dpr_lox_temps_2_t;

typedef struct {
    uint8_t is_recording;
    uint8_t power_state;
} payload_cam_status_t;

#pragma pack(pop)

#endif /* CAN_H */
