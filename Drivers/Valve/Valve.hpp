//    FILE: Valve.hpp
// PURPOSE: Common interface for all propulsion valves (solenoid on/off and
//          servo-driven proportional ball valves), plus the driver-level
//          implementations and the static list of every valve on the vehicle.
//
// Two actuation types are used on this vehicle (see valve list, mission doc):
//   - SolenoidValve   : GPIO on/off, drives a 24V solenoid coil (NC or NO).
//   - ServoBallValve  : PWM position control, drives a Savox SB-2290SG servo
//                        connected to a ball valve for dynamic pressure
//                        regulation (DPR).
//
// Both are exposed through the IValve interface so calling code (e.g. an
// abort sequence that must close every valve) can operate on a uniform list
// without caring which backend a given valve uses.

#ifndef DRIVERS_VALVE_VALVE_H
#define DRIVERS_VALVE_VALVE_H

#include "stm32h7xx_hal.h"

#include <cstdint>

// ---------------------------------------------------------------------------
// Common types
// ---------------------------------------------------------------------------

enum class ValveStatus : uint8_t {
    Ok,
    InvalidArgument,
    NotInitialized,
};

// ---------------------------------------------------------------------------
// IValve
// ---------------------------------------------------------------------------

class IValve {
public:
    virtual ~IValve() = default;

    // Fully opens (LOX/ethanol solenoids) or drives the servo to its open
    // endpoint (ball valves).
    virtual ValveStatus open() = 0;

    // Fully closes (solenoids) or drives the servo to its closed endpoint
    // (ball valves).
    virtual ValveStatus close() = 0;

    // Last commanded state. There is no position feedback on any of these
    // valves, so this reflects the last successful open()/close() call, not
    // a measured position.
    virtual bool is_open() const = 0;

    virtual const char* name() const = 0;
};

// ---------------------------------------------------------------------------
// SolenoidValve — GPIO on/off actuation
// ---------------------------------------------------------------------------
//
// Covers every "Solenoid, COTS" valve in the list (Buschjost / Jaksa, 24V).
// Each is either Normally Closed (NC, de-energized = closed) or Normally
// Open (NO, de-energized = open). The driver maps open()/close() to the
// correct GPIO level so callers always think in valve state, never coil
// energization.
//
// The GPIO here is assumed to drive a low-side MOSFET/relay switching the
// 24V coil, not the coil directly — confirm against the actual driver
// circuit before wiring active_high.

enum class FailSafeState : uint8_t {
    NormallyClosed, // de-energized -> closed (e.g. PR_VALVE_MO-nc)
    NormallyOpen,   // de-energized -> open   (e.g. PR_VALVE_VO-no)
};

struct SolenoidValveConfig {
    GPIO_TypeDef* port;
    uint16_t      pin;
    FailSafeState fail_safe;
    bool          active_high = true; // set false if the drive circuit inverts
    const char*   name        = "";
};

class SolenoidValve : public IValve {
public:
    explicit SolenoidValve(const SolenoidValveConfig& config);

    ValveStatus open() override;
    ValveStatus close() override;
    bool        is_open() const override;
    const char* name() const override;

    void setCallback(void (*callback)(bool old_open, bool new_open));

private:
    void set_energized(bool energized);

    SolenoidValveConfig config_;
    bool                is_open_;

    void (*callback_)(bool old_open, bool new_open) = nullptr;
};

// ---------------------------------------------------------------------------
// ServoBallValve — PWM proportional actuation
// ---------------------------------------------------------------------------
//
// Covers the two Savox SB-2290SG ball-valve/servo DPR units
// (PR_VALVE_PN_LOX, PR_VALVE_PN_ETH). Standard RC servo PWM: 50 Hz frame,
// pulse width in microseconds sets position. open_pulse_us/closed_pulse_us
// are the calibrated endpoints for this valve's mechanical travel — they are
// NOT necessarily the generic 1000/2000us RC range, so calibrate on the
// bench before flight.

struct ServoBallValveConfig {
    TIM_HandleTypeDef* htim;
    uint32_t           channel;         // e.g. TIM_CHANNEL_1

    // Optional separate enable/power GPIO for the servo (some servo wiring
    // switches the 8.4V rail independently of the PWM signal). Set
    // enable_port == nullptr to skip driving an enable line.
    GPIO_TypeDef* enable_port = nullptr;
    uint16_t      enable_pin  = 0;

    uint32_t            open_pulse_us   = 2100;
    uint32_t            closed_pulse_us = 900;
    const char*         name            = "";
};

class ServoBallValve : public IValve {
public:
    explicit ServoBallValve(const ServoBallValveConfig& config);

    ValveStatus open() override;
    ValveStatus close() override;
    bool        is_open() const override;
    const char* name() const override;

    // Proportional position for dynamic pressure regulation.
    // percent_open: 0.0 = fully closed, 100.0 = fully open.
    // dither: settle into the new position via a brief +/-1% wiggle first,
    // breaks static friction that otherwise makes the servo hunt/buzz at
    // the target. Pass false for the RST controller's continuous every-tick
    // tracking (PRESSURIZE_ON/REGULATE) -- dithering there would fight the
    // control loop instead of settling it. Leave it true (default) for
    // manual commands and one-shot state-entry positions.
    ValveStatus set_position(float percent_open, bool dither = true);
    
    void setCallback(void (*callback)(float old_open, float new_open));

private:
    void write_pulse_us(uint32_t pulse_us);

    ServoBallValveConfig config_;
    float                commanded_percent_open_;

    void (*callback_)(float old_open, float new_open) = nullptr;
};

#endif // DRIVERS_VALVE_VALVE_H
