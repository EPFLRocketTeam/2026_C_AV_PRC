#pragma once

#include "stm32h7xx_hal.h"
#include "Application/app_printf.h"
#include "Drivers/INA228/INA228.h"
#include <cstdint>

// 24 V rail monitor of a DPR board (LOX / ETH): one INA228 on I2C1.
//
// Blocking I2C (2 register reads per sample), so the sampling is rate-limited
// here and the caller just ticks it every loop.

#ifndef INA_24V_ADDR
#define INA_24V_ADDR 0x45
#endif
#ifndef INA_24V_PERIOD_MS
#define INA_24V_PERIOD_MS 100u
#endif
// Shunt fitted on the PRC: 13 mOhm. With ADCRANGE=0 the INA228 saturates at
// +-163.84 mV, i.e. 12.6 A at 13 mOhm -- keep MAX_CURRENT_A below that or the
// current reading clips. (Only the voltage is sent over CAN for now.)
#ifndef INA_24V_SHUNT_OHMS
#define INA_24V_SHUNT_OHMS 0.013f
#endif
#ifndef INA_24V_MAX_CURRENT_A
#define INA_24V_MAX_CURRENT_A 12.0f
#endif
// Consecutive failed reads before the rail is reported unhealthy.
#ifndef INA_24V_MAX_FAILS
#define INA_24V_MAX_FAILS 3
#endif

class Ina24vModule {
public:
    explicit Ina24vModule(I2C_HandleTypeDef *hi2c)
      : ina_(INA_24V_ADDR, hi2c) {}

    bool init() {
        ok_ = ina_.begin() &&
              (ina_.setMaxCurrentShunt(INA_24V_MAX_CURRENT_A, INA_24V_SHUNT_OHMS) == 0);
        if (ok_) {
            ina_.setAverage(Drivers::INA228::INA228_4_SAMPLES);
            ina_.setMode(Drivers::INA228::INA228_MODE_CONT_BUS_SHUNT);
        }
        fails_ = 0;
        app_printf("[INA-24V] addr 0x%02X: %s\n", INA_24V_ADDR, ok_ ? "ok" : "not found");
        return ok_;
    }

    // Returns true when a fresh sample was stored by this call.
    bool tick(uint32_t now_ms) {
        if (!ok_) return false;
        if (now_ms - last_ms_ < INA_24V_PERIOD_MS) return false;
        last_ms_ = now_ms;

        const float v = ina_.getBusVoltage();
        const bool v_err = ina_.getLastError() != 0;
        const float i = ina_.getCurrent();
        const bool i_err = ina_.getLastError() != 0;

        if (v_err || i_err) {
            if (++fails_ == INA_24V_MAX_FAILS) {
                app_printf("[INA-24V] %u consecutive read failures, rail unhealthy\n", fails_);
            }
            return false;
        }

        fails_ = 0;
        voltage_ = v;
        current_ = i;
        return true;
    }

    bool  healthy() const { return ok_ && fails_ < INA_24V_MAX_FAILS; }
    float voltage() const { return voltage_; }  // V
    float current() const { return current_; }  // A

private:
    Drivers::INA228::INA228_Driver ina_;
    bool     ok_ = false;
    uint32_t fails_ = 0;
    uint32_t last_ms_ = 0;
    float    voltage_ = 0.0f;
    float    current_ = 0.0f;
};
