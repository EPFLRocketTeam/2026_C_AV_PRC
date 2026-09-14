
#pragma once
#include "Modules/Sensors/sensor.hpp"
#include "Application/Data/data.hpp"

#include "ThirdParty/DataLogger/loggers/engine.hpp"
#include "ThirdParty/DataLogger/loggers/eth.hpp"
#include "ThirdParty/DataLogger/loggers/lox.hpp"

EngineDataLogger<PlumeStorage>& getEngineLogger();
EthDataLogger<PlumeStorage>& getEthLogger();
LoxDataLogger<PlumeStorage>& getLoxLogger();

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

const size_t TEMPERATURE_OTA_WINDOW_SIZE = 10;

#define PT1000_CHANNEL_PT1 ADC_CHANNEL_10
#define PT1000_CHANNEL_PT2 ADC_CHANNEL_11
#define PT1000_CHANNEL_PT3 ADC_CHANNEL_0
#define PT1000_CHANNEL_PT4 ADC_CHANNEL_1

#define PT1000_CHANNEL_T_OTA1 PT1000_CHANNEL_PT2
#define PT1000_CHANNEL_T_OTA2 PT1000_CHANNEL_PT3
#define PT1000_CHANNEL_T_OTA3 PT1000_CHANNEL_PT4
#define PT1000_CHANNEL_T_OTA4 PT1000_CHANNEL_PT1
#define PT1000_CHANNEL_T_OTA5 PT1000_CHANNEL_PT2
#define PT1000_CHANNEL_T_EIN PT1000_CHANNEL_PT3
#define PT1000_CHANNEL_T_OIN PT1000_CHANNEL_PT4

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
