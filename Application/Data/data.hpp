
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

struct PropSensors {
  double pressure_C;
  double pressure_C_mean;
  double temperature_C;
  double temperature_C_mean;
  double pressure_OIN;
  double pressure_OIN_mean;
  double pressure_EIN;
  double pressure_EIN_mean;
  double temperature_EIN;
  double temperature_EIN_mean;
  double temperature_OIN;
  double temperature_OIN_mean;

  double pressure_OTA;
  double pressure_OTA_mean;
  double pressure_HPO;
  double pressure_HPO_mean;
  double temperature_OTA[4];
  double temperature_OTA_mean[4];
  double FLS;
  double FLS_mean;

  double pressure_ETA;
  double pressure_ETA_mean;
  double pressure_HPE;
  double pressure_HPE_mean;

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

  double get_pressure_C() const;
  void set_pressure_C(double value);
  double get_pressure_C_mean() const;
  void set_pressure_C_mean(double value);

  double get_temperature_C() const;
  void set_temperature_C(double value);
  double get_temperature_C_mean() const;
  void set_temperature_C_mean(double value);

  double get_pressure_OIN() const;
  void set_pressure_OIN(double value);
  double get_pressure_OIN_mean() const;
  void set_pressure_OIN_mean(double value);

  double get_pressure_EIN() const;
  void set_pressure_EIN(double value);
  double get_pressure_EIN_mean() const;
  void set_pressure_EIN_mean(double value);

  double get_temperature_EIN() const;
  void set_temperature_EIN(double value);
  double get_temperature_EIN_mean() const;
  void set_temperature_EIN_mean(double value);

  double get_temperature_OIN() const;
  void set_temperature_OIN(double value);
  double get_temperature_OIN_mean() const;
  void set_temperature_OIN_mean(double value);

  double get_pressure_OTA() const;
  void set_pressure_OTA(double value);
  double get_pressure_OTA_mean() const;
  void set_pressure_OTA_mean(double value);

  double get_pressure_HPO() const;
  void set_pressure_HPO(double value);
  double get_pressure_HPO_mean() const;
  void set_pressure_HPO_mean(double value);

  double get_temperature_OTA(uint8_t sensor_index) const;
  void set_temperature_OTA(uint8_t sensor_index, double value);
  double get_temperature_OTA_mean(uint8_t sensor_index) const;
  void set_temperature_OTA_mean(uint8_t sensor_index, double value);

  double get_FLS() const;
  void set_FLS(double value);
  double get_FLS_mean() const;
  void set_FLS_mean(double value);

  double get_pressure_ETA() const;
  void set_pressure_ETA(double value);
  double get_pressure_ETA_mean() const;
  void set_pressure_ETA_mean(double value);

  double get_pressure_HPE() const;
  void set_pressure_HPE(double value);
  double get_pressure_HPE_mean() const;
  void set_pressure_HPE_mean(double value);
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
