
#pragma once
#include "Modules/Sensors/sensor.hpp"

const size_t DEFAULT_NUMBER_TICKS = 10;

using CommonTimerPolicy = TimerPollPolicy<DEFAULT_NUMBER_TICKS>;

template<auto Field>
struct PrcStorage {
    auto& ref () const {
        return (prc::PrcStore::get_instance().*Field);
    }
};

#define ENGINE_SETTER_POLICY(member) StoreSetterPolicy<PrcStorage<\
    &prc::PrcStore::propSensorsStoreEngine>, &member>
#define ETH_SETTER_POLICY(member) StoreSetterPolicy<PrcStorage<\
    &prc::PrcStore::propSensorsStoreEth>, &member>
#define LOX_SETTER_POLICY(member) StoreSetterPolicy<PrcStorage<\
    &prc::PrcStore::propSensorsStoreLox>, &member>
