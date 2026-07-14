#include "Drivers/Valve/Valve.hpp"

// ---------------------------------------------------------------------------
// SolenoidValve
// ---------------------------------------------------------------------------

SolenoidValve::SolenoidValve(const SolenoidValveConfig& config)
    : config_(config),
      is_open_(config.fail_safe == FailSafeState::NormallyOpen)
{
}

void SolenoidValve::set_energized(bool energized)
{
    GPIO_PinState level = energized == config_.active_high ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(config_.port, config_.pin, level);
}

ValveStatus SolenoidValve::open()
{
    // NC valve opens when energized; NO valve opens when de-energized.
    bool energize = (config_.fail_safe == FailSafeState::NormallyClosed);
    set_energized(energize);
    is_open_ = true;
    return ValveStatus::Ok;
}

ValveStatus SolenoidValve::close()
{
    // NC valve closes when de-energized; NO valve closes when energized.
    bool energize = (config_.fail_safe == FailSafeState::NormallyOpen);
    set_energized(energize);
    is_open_ = false;
    return ValveStatus::Ok;
}

bool SolenoidValve::is_open() const
{
    return is_open_;
}

const char* SolenoidValve::name() const
{
    return config_.name;
}

// ---------------------------------------------------------------------------
// ServoBallValve
// ---------------------------------------------------------------------------

ServoBallValve::ServoBallValve(const ServoBallValveConfig& config)
    : config_(config),
      commanded_percent_open_(0.0f)
{
    if (config_.enable_port != nullptr) {
        HAL_GPIO_WritePin(config_.enable_port, config_.enable_pin, GPIO_PIN_SET);
    }
    // HAL_TIM_PWM_ConfigChannel() (in MX_TIM4_Init) only loads the channel's
    // config registers — it doesn't enable the output. Without this call the
    // compare register updates in write_pulse_us() never reach the pin.
    HAL_TIM_PWM_Start(config_.htim, config_.channel);
}

void ServoBallValve::write_pulse_us(uint32_t pulse_us)
{
    // Assumes the timer's tick period is configured for 1 us resolution
    // (ARR/PSC set for a 1 MHz counter clock), so the compare value is the
    // pulse width directly in microseconds.
    __HAL_TIM_SET_COMPARE(config_.htim, config_.channel, pulse_us);
}

ValveStatus ServoBallValve::open()
{
    write_pulse_us(config_.open_pulse_us);
    commanded_percent_open_ = 100.0f;
    return ValveStatus::Ok;
}

ValveStatus ServoBallValve::close()
{
    write_pulse_us(config_.closed_pulse_us);
    commanded_percent_open_ = 0.0f;
    return ValveStatus::Ok;
}

bool ServoBallValve::is_open() const
{
    return commanded_percent_open_ > 0.0f;
}

const char* ServoBallValve::name() const
{
    return config_.name;
}

ValveStatus ServoBallValve::set_position(float percent_open)
{
    if (percent_open < 0.0f || percent_open > 100.0f) {
        return ValveStatus::InvalidArgument;
    }

    float ratio = percent_open / 100.0f;
    int32_t span = static_cast<int32_t>(config_.open_pulse_us) -
                    static_cast<int32_t>(config_.closed_pulse_us);
    uint32_t pulse_us = config_.closed_pulse_us + static_cast<uint32_t>(span * ratio);

    write_pulse_us(pulse_us);
    commanded_percent_open_ = percent_open;
    return ValveStatus::Ok;
}
