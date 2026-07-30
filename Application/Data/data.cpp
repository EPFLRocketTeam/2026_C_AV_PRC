#include "Application/Data/data.hpp"

#include "stm32h7xx_hal.h"

using namespace prc;

StateStore::StateStore() { data_ = State::MANUAL; }

const DataDump &PrcStore::get() const {
  data_.prc_state         = stateStore.get();
  data_.prc_timestamp_ms  = HAL_GetTick();
  data_.boardIdentity     = boardIdentityStore.get();
  data_.valves            = valvesStore.get();
  data_.intranetCmd         = intranetCmdStore.get();
  data_.event             = eventStore.get();
  return data_;
}

void PrcStore::set(const DataDump &value) {
  stateStore.set(value.prc_state);
  boardIdentityStore.set(value.boardIdentity);
  valvesStore.set(value.valves);
  intranetCmdStore.set(value.intranetCmd);
  eventStore.set(value.event);
}

DataDump *PrcStore::get_ref() { return &data_; }
