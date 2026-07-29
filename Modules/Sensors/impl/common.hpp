
#pragma once
#include "Modules/Sensors/sensor.hpp"
#include "Application/Data/data.hpp"

const size_t DEFAULT_NUMBER_TICKS = 10;

/* Channel Index of sensata SENS1 */
const uint8_t SENSATA_CHANNEL_L1 = 1;
/* Channel Index of sensata SENS2 */
const uint8_t SENSATA_CHANNEL_L2 = 0;
/* Channel Index of sensata SENS3 */
const uint8_t SENSATA_CHANNEL_L3 = 2;
/* Channel Index of sensata SENS4 
 *   TODO test that it works. Shorted on V1
 */
const uint8_t SENSATA_CHANNEL_L4 = 6;
/* Channel Index of sensata SENS5 */
const uint8_t SENSATA_CHANNEL_L5 = 5;

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
