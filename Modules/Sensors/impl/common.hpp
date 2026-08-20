
#pragma once
#include "Modules/Sensors/sensor.hpp"
#include "Application/Data/data.hpp"

#include "ThirdParty/DataLogger/loggers/engine.hpp"
#include "ThirdParty/DataLogger/loggers/eth.hpp"
#include "ThirdParty/DataLogger/loggers/lox.hpp"

extern EngineDataLogger<PlumeStorage> engineLogger;
extern EthDataLogger<PlumeStorage> ethLogger;
extern LoxDataLogger<PlumeStorage> loxLogger;

const size_t DEFAULT_NUMBER_TICKS = 10;

/* Channel Index of sensata SENS1 */
const uint8_t SENSATA_CHANNEL_L1 = 6;
/* Channel Index of sensata SENS2 */
const uint8_t SENSATA_CHANNEL_L2 = 5;
/* Channel Index of sensata SENS3 */
const uint8_t SENSATA_CHANNEL_L3 = 3;
/* Channel Index of sensata SENS4 */
const uint8_t SENSATA_CHANNEL_L4 = 2;
/* Channel Index of sensata SENS5 */
const uint8_t SENSATA_CHANNEL_L5 = 1;

#define PT1000_CHANNEL_T_EIN ADC_CHANNEL_2 // TODO: placeholder, confirm real ADC channel from schematic/.ioc
#define PT1000_CHANNEL_T_OIN ADC_CHANNEL_3 // TODO: placeholder, confirm real ADC channel from schematic/.ioc

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
