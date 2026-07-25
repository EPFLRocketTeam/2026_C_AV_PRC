
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/engine/consts.hpp"

using PressureHpeRawSetter  = ETH_SETTER_POLICY(prc::PropSensorsStoreEth::set_pressure_HPE);
using PressureHpeMeanSetter = ETH_SETTER_POLICY(prc::PropSensorsStoreEth::set_pressure_HPE_mean);
using PressureHpeMeanFilter = RunningAveragePolicy<PRESSURE_HPE_WINDOW_SIZE, PressureHpeMeanSetter>;
using PressureHpeRawBranch  = BranchSetterPolicy<PressureHpeRawSetter, PressureHpeMeanFilter>;

using PressureHpeSensorModule = SensorModule<CommonTimerPolicy, SampleSensorModule, PressureHpeRawBranch>;
