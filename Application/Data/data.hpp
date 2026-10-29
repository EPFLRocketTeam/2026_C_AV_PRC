
#ifndef APP_PRC_DATA_H
#define APP_PRC_DATA_H

#include <cstdint>

#include "ThirdParty/DataLogger/Client/annotations.hpp"
#include "Application/Data/fsm.hpp"
#include "./istore.hpp"
#include "./propulsion/data.hpp"


namespace prc {

// ---------------------------------------------------------------------------
// Board identity — which of the 3 physical "PRC" boards this firmware image
// is running on: the Dynamic Pressure Regulator for LOx, the DPR for
// Ethanol, or the Engine Bay PRC
//
// Determined at boot from a single strap/ID pin (see Drivers/PrcBoardId —
// pin TBD) see Prc_DetectBoardRole().
// ---------------------------------------------------------------------------

enum class BoardRole : uint8_t {
  Unknown,   // detection not run yet
  DprLox,
  DprEth,
  EngineBay,
};
static_assert(sizeof(BoardRole) == 1);

struct BoardIdentity {
  BoardRole role;

  BoardIdentity();
};

struct Valves {
  bool valve_safety_lox;
  bool valve_safety_fuel;

  bool valve_vent_lox;
  bool valve_vent_fuel;

  bool valve_main_lox;
  bool valve_main_fuel;

  CSV_IGNORE
  uint8_t padding[2];

  float ball_valve_lox;
  float ball_valve_fuel;

  Valves();
};
static_assert(sizeof(Valves) == 16);

// ---------------------------------------------------------------------------
// Command received from the Flight Computer over the CAN link
// (2026_C_AV_PRC <-> 2026_C_AV_FC, see CANBUS_TEST_* in Core/Src/main.c).
// Mirrors FC's UplinkCmd struct shape (same fields; named after the PRC<->FC
// intranet here, since on this board the command arrives over that link).
// ---------------------------------------------------------------------------

struct IntranetCmd {
  uint16_t id;
  uint8_t value;

  CSV_IGNORE
  uint8_t padding;

  IntranetCmd();
};
static_assert(sizeof(IntranetCmd) == 4);

// ---------------------------------------------------------------------------
// Derived/latched conditions the FSM transitions on. Same division of
// responsibility as flight_computer::Event, but left empty for now — the
// command protocol, timing values, and pressure bounds it would depend on
// aren't defined yet. Fill in once those are confirmed.
// ---------------------------------------------------------------------------

struct Event {
  bool no_cable_continuity;

  Event();
};

class StateStore : public IStore<State> {
public:
  StateStore();
};

// Read-only from the outside: the board's role is a hardware fact (which
// physical board this firmware is flashed to), latched once at boot by
// Prc_Fsm_Init() via the inherited IStore<T>::set(), not a value application
// code should be able to casually reassign — hence no set_role() here.
class BoardIdentityStore : public IStore<BoardIdentity> {
public:
  BoardIdentityStore();

  BoardRole get_role() const;
};

class ValvesStore : public IStore<Valves> {
public:
  ValvesStore();

  bool get_valve_safety_lox () const;
  void set_valve_safety_lox(bool value);
  
  bool get_valve_safety_fuel () const;
  void set_valve_safety_fuel(bool value);
  
  bool get_valve_vent_lox () const;
  void set_valve_vent_lox(bool value);
  
  bool get_valve_vent_fuel () const;
  void set_valve_vent_fuel(bool value);
  
  bool get_valve_main_lox () const;
  void set_valve_main_lox(bool value);
  
  bool get_valve_main_fuel () const;
  void set_valve_main_fuel(bool value);
  
  float get_ball_valve_lox () const;
  void set_ball_valve_lox(float value);
  
  float get_ball_valve_fuel () const;
  void set_ball_valve_fuel(float value);
};

class IntranetCmdStore : public IStore<IntranetCmd> {
public:
  IntranetCmdStore();

  uint16_t get_id() const;
  void set_id(uint16_t value);

  uint8_t get_value() const;
  void set_value(uint8_t value);
};

// Empty for now — see Event's comment. Kept as a class (rather than removed)
// so PrcStore/DataDump's shape doesn't change once fields are added back.
class EventStore : public IStore<Event> {
public:
  EventStore();

  bool get_no_cable_continuity () const;
  void set_no_cable_continuity (bool value);
};

// ---------------------------------------------------------------------------
// FC liveness, fed by the heartbeat_fc CAN callback (prc_can.cpp). Exposed
// only: no FSM reacts to FC silence. Kept out of DataDump so the logged
// layout does not move. HAL free, so it is host tested.
// ---------------------------------------------------------------------------

struct FcLink {
  uint32_t last_counter = 0;
  uint32_t last_rx_ms   = 0;
  bool     seen         = false;
  bool     reported_alive = false; // last state printed
  bool     grace_done     = false;
};

class FcLinkStore : public IStore<FcLink> {
public:
  static constexpr uint32_t TIMEOUT_MS = 500;
  static constexpr uint32_t BOOT_GRACE_MS = 2000;

  inline void on_heartbeat(uint32_t counter, uint32_t now_ms) {
    data_.last_counter = counter;
    data_.last_rx_ms   = now_ms;
    data_.seen         = true;
  }

  // True when a heartbeat arrived less than TIMEOUT_MS ago. Unsigned
  // subtraction keeps it right across the HAL_GetTick() wrap.
  inline bool alive(uint32_t now_ms) const {
    return data_.seen && (now_ms - data_.last_rx_ms) < TIMEOUT_MS;
  }

  // True when `alive` should be printed now: on every alive or dead edge,
  // and once as "dead" when now_ms (ms since boot) reaches BOOT_GRACE_MS
  // with the FC never heard from.
  inline bool take_report(uint32_t now_ms, bool alive) {
    bool report = (alive != data_.reported_alive);
    data_.reported_alive = alive;
    if (!data_.grace_done && now_ms >= BOOT_GRACE_MS) {
      data_.grace_done = true;
      if (!data_.seen) report = true;
    }
    return report;
  }
};

struct DataDump {
  State          prc_state;
  CSV_IGNORE
  uint8_t        pad_state[7];

  uint32_t       prc_timestamp_ms;
  BoardIdentity  boardIdentity;
  Valves         valves;
  IntranetCmd    intranetCmd;
  Event          event;

  PropSensorsEngine propSensorsEngine;
  PropSensorsEth    propSensorsEth;
  PropSensorsLox    propSensorsLox;
};

static_assert(offsetof(DataDump, prc_state) == 0);
static_assert(offsetof(DataDump, pad_state) == 1);
static_assert(offsetof(DataDump, prc_timestamp_ms) == 8);
static_assert(offsetof(DataDump, boardIdentity) == 12);
static_assert(offsetof(DataDump, valves) == 16);
static_assert(offsetof(DataDump, intranetCmd) == 32);
static_assert(offsetof(DataDump, event) == 36);
static_assert(sizeof(DataDump) == 40 + 112 + 96 + 48 + 16 + 16 + 16);

// Aggregating singleton — mirrors flight_computer::GOATStore.
class PrcStore {
public:
  StateStore         stateStore;
  BoardIdentityStore boardIdentityStore;
  ValvesStore        valvesStore;
  IntranetCmdStore     intranetCmdStore;
  EventStore         eventStore;
  FcLinkStore        fcLinkStore;

  PropSensorsStoreEngine propSensorsStoreEngine;
  PropSensorsStoreEth    propSensorsStoreEth;
  PropSensorsStoreLox    propSensorsStoreLox;

  void set(const DataDump &value);
  const DataDump &get(uint32_t timestamp) const;
  DataDump *get_ref();

  static inline PrcStore &get_instance() {
    static PrcStore instance;
    return instance;
  }

private:
  PrcStore() = default;
  mutable DataDump data_;
};

}; // namespace prc

#endif /* APP_PRC_DATA_H */
