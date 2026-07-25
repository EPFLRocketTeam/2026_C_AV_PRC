
#pragma once

#include "sigutils/outlier.hpp"
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/engine/consts.hpp"

// =========================================================================================================
// ========================================== PRESSURE ETA{1,2,3} ==========================================
// =========================================================================================================

struct PressureEtaFrame {
    /* Array of pressures from the PRESSURE_ETA_NUMBER_SENSORS sensors */
    double pressures[PRESSURE_ETA_NUMBER_SENSORS];
    /* Whether the sensor is an outlier */
    bool is_outlier[PRESSURE_ETA_NUMBER_SENSORS];

    /* Number of used data points */
    size_t number_used;
    /* Final pressure computed */
    double pressure;
};

template<typename SingleSensorModule>
struct PressureEtaSensors {
private:
    SingleSensorModule etaSensor1;
    SingleSensorModule etaSensor2;
    SingleSensorModule etaSensor3;
public:
    SingleSensorModule& getSensor1 () { return etaSensor1; }
    SingleSensorModule& getSensor2 () { return etaSensor2; }
    SingleSensorModule& getSensor3 () { return etaSensor3; }

    PressureEtaFrame poll () {
        PressureEtaFrame frame;
        frame.pressures[0] = etaSensor1.poll();
        frame.pressures[1] = etaSensor2.poll();
        frame.pressures[2] = etaSensor3.poll();
        
        return frame;
    }
};

template<typename ForwardType>
struct PressureEtaOutlierFilter {
private:
    ForwardType forward;

    OutlierParams<double, 3, PRESSURE_ETA_NUMBER_KEPT> params;
public:
    PressureEtaOutlierFilter () {
        params.min = PRESSURE_ETA_MIN_VALUE;
        params.max = PRESSURE_ETA_MAX_VALUE;
    }

    void set (PressureEtaFrame frame) {
        frame.number_used = outlier(params, frame.pressures, frame.is_outlier, frame.pressure);

        forward.set(frame);
    }
};

template<typename ForwardType>
struct PressureEtaEnoughDataPointsCheck {
private:
    ForwardType forward;
public:
    void set (const PressureEtaFrame &frame) {
        // Drop the frame
        if (frame.number_used < PRESSURE_ETA_MIN_NUMBER) {
            return ;
        }

        forward.set(frame);
    }
};

template<const size_t Index, typename ForwardType>
struct PressureEtaUnpackRawPressure {
private:
    ForwardType forward;
public:
    void set (const PressureEtaFrame &frame) {
        forward.set(frame.pressures[Index]);
    }
};

template<typename ForwardType>
struct PressureEtaUnpackPressure {
private:
    ForwardType forward;
public:
    void set (const PressureEtaFrame &frame) {
        forward.set(frame.pressure);
    }
};

// ========================================================================================================
// =============================================== PIPELINE ===============================================
// ========================================================================================================

// raw setters pipeline
using PressureEtaRawSetter1 = ETH_SETTER_POLICY(prc::PropSensorsStoreEth::set_pressure_ETA1);
using PressureEtaRawSetter2 = ETH_SETTER_POLICY(prc::PropSensorsStoreEth::set_pressure_ETA2);
using PressureEtaRawSetter3 = ETH_SETTER_POLICY(prc::PropSensorsStoreEth::set_pressure_ETA3);

using PressureEtaRawFromFrameSetter1 = PressureEtaUnpackRawPressure<0, PressureEtaRawSetter1>;
using PressureEtaRawFromFrameSetter2 = PressureEtaUnpackRawPressure<1, PressureEtaRawSetter2>;
using PressureEtaRawFromFrameSetter3 = PressureEtaUnpackRawPressure<2, PressureEtaRawSetter3>;

// mean data pipeline
using PressureEtaMeanSetter = ETH_SETTER_POLICY(prc::PropSensorsStoreEth::set_pressure_ETA_mean);
using PressureEtaMeanFilter = RunningAveragePolicy<PRESSURE_ETA_WINDOW_SIZE, PressureEtaMeanSetter>;
using PressureEtaMeanUnpacker = PressureEtaUnpackPressure<PressureEtaMeanFilter>;

using PressureEtaEnoughDataPointsForMean = PressureEtaEnoughDataPointsCheck<PressureEtaMeanUnpacker>;

// combined pipeline
using PressureEtaPipeline = BranchSetterPolicy<
    PressureEtaRawFromFrameSetter1,
    PressureEtaRawFromFrameSetter2,
    PressureEtaRawFromFrameSetter3,
    PressureEtaEnoughDataPointsForMean
>;
using PressureEtaOutlierPipeline = PressureEtaOutlierFilter<PressureEtaPipeline>;

using PressureEtaSensorModule = SensorModule<
    CommonTimerPolicy,
    PressureEtaSensors<SampleSensorModule>,
    PressureEtaOutlierPipeline
>;
