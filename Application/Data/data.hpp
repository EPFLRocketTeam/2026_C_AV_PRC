
#ifndef APP_PRC_DATA_H
#define APP_PRC_DATA_H

#include <cstdint>

#include "Application/Data/fsm.hpp"

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

struct BoardIdentity {
  BoardRole role;

  BoardIdentity();
};

// ---------------------------------------------------------------------------
// PropSensors / Valves — same struct shape as 2026_C_AV_FC's
// Application/Data/data.hpp (field-for-field), not a PRC-specific redesign.
// This board only ever populates/uses the subset relevant to whichever of
// the 3 board roles it is (DprLox/DprEth/EngineBay — see BoardRole above);
// the rest sit at their default (0.0/false) values. Kept as the full FC
// shape rather than a filtered subset so this board's data lines up
// directly with what FC already expects to receive/send for these domains.
//
// Sensor sources for the DPR-relevant fields (LOX/fuel pressure+level+temp):
//   Drivers/SensataPte7300 (pressure + the sensor die's own bridge temp),
//   Drivers/FDC1004 (FLS — capacitive level sensing), Drivers/PT1000
//   (dedicated tank/fluid temperature probe). TODO: sensor -> field mapping
//   (which physical sensor feeds which of *_level/*_temperature below) not
//   yet confirmed.
// ---------------------------------------------------------------------------

struct PropSensors {
  // --- DPR-relevant (LOX or Eth board populates these) ---
  // Pressure fields have a confirmed mapping in prc_state.cpp
  // (CurrentTankPressureBar()/CurrentCopvPressureBar()); which physical
  // FDC1004/PT1000 channel feeds *_level/*_temperature below is still an
  // open TODO (see the sensor-sources comment above). None of these are
  // actually populated by any driver yet either way -- see
  // PropSensorsStore's usage (or lack of it).
  double N2_pressure;
  double N2_pressure_mean;

  double fuel_pressure;
  double fuel_pressure_mean;

  double LOX_pressure;
  double LOX_pressure_mean;

  double fuel_level;
  double LOX_level;

  double N2_temperature;
  double fuel_temperature;
  double LOX_temperature;

  // --- Engine Bay only -- a DPR (LOX/Eth) board never populates these ---
  // Injector/chamber/igniter pressures and temperatures only make sense at
  // the engine, not the propellant tanks. Kept here only because this
  // struct mirrors FC's full shape rather than being a DPR-specific subset
  // (see the comment above).
  double igniter_pressure;
  double LOX_inj_pressure;
  double fuel_inj_pressure;
  double chamber_pressure;
  double igniter_temperature;
  double fuel_inj_temperature;
  double fuel_inj_cooling_temperature;
  double LOX_inj_temperature;
  double chamber_temperature;

  // --- Generic/shared -- not tied to a specific sensor or board role ---
  uint32_t PR_state;
  uint32_t timer_burn;

  PropSensors();
};

struct Valves {
  bool valve_dpr_pressure_lox;
  bool valve_dpr_pressure_fuel;
  bool valve_dpr_vent_copv;
  bool valve_dpr_vent_lox;
  bool valve_dpr_vent_fuel;
  bool valve_prb_main_lox;
  bool valve_prb_main_fuel;

  Valves();
};

// ---------------------------------------------------------------------------
// Command received from the Flight Computer over the CAN link
// (2026_C_AV_PRC <-> 2026_C_AV_FC, see CANBUS_TEST_* in Core/Src/main.c).
// Mirrors FC's UplinkCmd struct shape for consistency across the two repos.
// ---------------------------------------------------------------------------

struct UplinkCmd {
  uint8_t id;
  uint8_t value;

  UplinkCmd();
};

// ---------------------------------------------------------------------------
// Derived/latched conditions the FSM transitions on. Same division of
// responsibility as flight_computer::Event, but left empty for now — the
// command protocol, timing values, and pressure bounds it would depend on
// aren't defined yet. Fill in once those are confirmed.
// ---------------------------------------------------------------------------

struct Event {
  Event();
};

// ---------------------------------------------------------------------------
// IStore<T> — identical shape to flight_computer::IStore<T>.
// ---------------------------------------------------------------------------

template <typename T> class IStore {
public:
  virtual ~IStore() = default;

  inline void set(const T &value) { data_ = value; };
  inline const T &get() const { return data_; };
  inline T *get_ref() { return &data_; };

protected:
  T data_;
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

class PropSensorsStore : public IStore<PropSensors> {
public:
  PropSensorsStore();

  double get_N2_pressure() const;
  void set_N2_pressure(double value);

  double get_fuel_pressure() const;
  void set_fuel_pressure(double value);

  double get_LOX_pressure() const;
  void set_LOX_pressure(double value);

  double get_igniter_pressure() const;
  void set_igniter_pressure(double value);

  double get_LOX_inj_pressure() const;
  void set_LOX_inj_pressure(double value);

  double get_fuel_inj_pressure() const;
  void set_fuel_inj_pressure(double value);

  double get_chamber_pressure() const;
  void set_chamber_pressure(double value);

  double get_fuel_level() const;
  void set_fuel_level(double value);

  double get_LOX_level() const;
  void set_LOX_level(double value);

  double get_N2_temperature() const;
  void set_N2_temperature(double value);

  double get_fuel_temperature() const;
  void set_fuel_temperature(double value);

  double get_LOX_temperature() const;
  void set_LOX_temperature(double value);

  double get_igniter_temperature() const;
  void set_igniter_temperature(double value);

  double get_fuel_inj_temperature() const;
  void set_fuel_inj_temperature(double value);

  double get_fuel_inj_cooling_temperature() const;
  void set_fuel_inj_cooling_temperature(double value);

  double get_LOX_inj_temperature() const;
  void set_LOX_inj_temperature(double value);

  double get_chamber_temperature() const;
  void set_chamber_temperature(double value);

  uint32_t get_PR_state() const;
  void set_PR_state(uint32_t value);
};

class ValvesStore : public IStore<Valves> {
public:
  ValvesStore();

  bool get_valve_dpr_pressure_lox() const;
  void set_valve_dpr_pressure_lox(bool value);

  bool get_valve_dpr_pressure_fuel() const;
  void set_valve_dpr_pressure_fuel(bool value);

  bool get_valve_dpr_vent_copv() const;
  void set_valve_dpr_vent_copv(bool value);

  bool get_valve_dpr_vent_lox() const;
  void set_valve_dpr_vent_lox(bool value);

  bool get_valve_dpr_vent_fuel() const;
  void set_valve_dpr_vent_fuel(bool value);

  bool get_valve_prb_main_lox() const;
  void set_valve_prb_main_lox(bool value);

  bool get_valve_prb_main_fuel() const;
  void set_valve_prb_main_fuel(bool value);
};

class UplinkCmdStore : public IStore<UplinkCmd> {
public:
  UplinkCmdStore();

  uint8_t get_id() const;
  void set_id(uint8_t value);

  uint8_t get_value() const;
  void set_value(uint8_t value);
};

// Empty for now — see Event's comment. Kept as a class (rather than removed)
// so PrcStore/DataDump's shape doesn't change once fields are added back.
class EventStore : public IStore<Event> {
public:
  EventStore();
};

struct DataDump {
  State          prc_state;
  uint32_t       prc_timestamp_ms;
  BoardIdentity  boardIdentity;
  PropSensors    propSensors;
  Valves         valves;
  UplinkCmd      uplinkCmd;
  Event          event;
};

// Aggregating singleton — mirrors flight_computer::GOATStore.
class PrcStore {
public:
  StateStore         stateStore;
  BoardIdentityStore boardIdentityStore;
  PropSensorsStore   propSensorsStore;
  ValvesStore        valvesStore;
  UplinkCmdStore     uplinkCmdStore;
  EventStore         eventStore;

  void set(const DataDump &value);
  const DataDump &get() const;
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
