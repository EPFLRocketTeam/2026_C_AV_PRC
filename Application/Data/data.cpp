#include "Application/Data/data.hpp"

#include "stm32h7xx_hal.h"

using namespace prc;

StateStore::StateStore() { data_ = State::MANUAL; }

const DataDump &PrcStore::get() const {
  data_.prc_state         = stateStore.get();
  data_.prc_timestamp_ms  = HAL_GetTick();
  data_.boardIdentity     = boardIdentityStore.get();
  data_.propSensors       = propSensorsStore.get();
  data_.valves            = valvesStore.get();
  data_.uplinkCmd         = uplinkCmdStore.get();
  data_.event             = eventStore.get();
  return data_;
}

void PrcStore::set(const DataDump &value) {
  stateStore.set(value.prc_state);
  boardIdentityStore.set(value.boardIdentity);
  propSensorsStore.set(value.propSensors);
  valvesStore.set(value.valves);
  uplinkCmdStore.set(value.uplinkCmd);
  eventStore.set(value.event);
}

DataDump *PrcStore::get_ref() { return &data_; }
