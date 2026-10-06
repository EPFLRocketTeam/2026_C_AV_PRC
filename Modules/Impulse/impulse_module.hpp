
#pragma once

#include <cstdint>
#include <cstddef>

#include "sigutils/integrator.hpp"
#include "Application/Data/data.hpp"
#include "Application/Config/config.hpp"

template<auto GetTickMs>
struct ImpulseModule {
private:
    enum ImpulseModuleState {
        INIT,
        PRE_STARTED,
        STARTED,
        STOPPED
    };

    ImpulseModuleState module_state_ = INIT;

    /* Atmospheric pressure as detected by the mean value of the sensata reading */
    float pressure_C_offset_ = 0;
    /* Current mean value of the sensata reading */
    float pressure_C_mean_ = 0;

    /* Current reading of the sensata */
    float pressure_C_ = 0;
    
    StableIntegratorMillis<float> integrator_;
    
    bool transitionState (ImpulseModuleState src, ImpulseModuleState dst) {
        if (module_state_ != src) {
            return false;
        }

        module_state_ = dst;
        return true;
    }
public:
    void reset () {
        module_state_ = INIT;

        pressure_C_offset_ = pressure_C_mean_;
    }

    /* Should be called at least 10 seconds before start
     *  to prepare the mean chamber pressure as a way of
     *  measuring the sensor offset.
     */
    bool preStart () {
        return transitionState(INIT, PRE_STARTED);
    }
    bool start () {
        if (!transitionState(PRE_STARTED, STARTED)) {
            return false;
        }

        integrator_.start(pressure_C_, GetTickMs());
        return true;
    }
    bool stop () {
        return transitionState(STARTED, STOPPED);
    }
    
    void ingestChamberPressureMean (float pressure_C_mean) {
        pressure_C_mean_ = pressure_C_mean;
        
        if (module_state_ == INIT) {
            pressure_C_offset_ = pressure_C_mean_;
        }
    }
    void ingestChamberPressure (float pressure_C) {
        pressure_C -= pressure_C_offset_;
        
        if (pressure_C < 0) {
            pressure_C = 0.;
        }

        pressure_C_ = pressure_C;

        if (module_state_ != STARTED) {
            return ;
        }

        integrator_.ingest(pressure_C_, GetTickMs());

        float pressure_C_integral = (float) integrator_;

        prc::PrcStore::get_instance()
            .propSensorsStoreEngine
            .set_integral_pressure_C(pressure_C_integral);
        
        prc::PrcStore::get_instance()
            .propSensorsStoreEngine
            .set_total_engine_impuse(pressure_C_integral * config::get().Burn.PressureIntegralToImpulse);
    }
};
