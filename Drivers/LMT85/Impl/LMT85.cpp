#include "../LMT85.hpp"

#include <cmath>

namespace Drivers {
namespace LMT85 {

LMT85Driver::LMT85Driver(Config config)
    : config_(config) {}

bool LMT85Driver::init() {
    return config_.hadc != nullptr;
}

namespace {
// Raw single-shot reads at the shortest sample time swung ~1600 LSB
// (~10 degC) tick to tick -- far more than real thermal drift, i.e. ADC/
// noise, not signal. Longer acquisition time (was 1.5 cycles) gives the
// sample-and-hold cap more time to settle; averaging further smooths
// whatever noise remains. Cheap fix -- a proper one would also add an
// output filter cap at the sensor per the LMT85 datasheet's own
// recommendation, but that's a hardware change, not a firmware one.
constexpr uint32_t kSamplingTime = ADC_SAMPLETIME_64CYCLES_5;
// Bumped 16 -> 128 as a diagnostic: 16-sample averaging barely reduced the
// swing, which for independent random noise should have cut it by ~4x
// (sqrt(16)) -- pointing at correlated/systematic noise (wiring, missing
// output filter cap, supply ripple) rather than something more averaging
// alone fixes. If 128 samples doesn't help much either, this confirms it's
// a hardware issue, not a firmware one.
constexpr int      kNumSamples   = 128;
} // namespace

bool LMT85Driver::read(LMT85Data& out) {
    out.valid   = false;
    out.raw_adc = 0;

    if (config_.hadc == nullptr) {
        return false;
    }

    // Point the ADC's regular rank-1 channel at this sensor before
    // converting, same reasoning as PT1000Driver::read(): without this,
    // HAL_ADC_Start() just re-converts whatever channel was last configured.
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel                = config_.adc_channel;
    sConfig.Rank                   = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime           = kSamplingTime;
    sConfig.SingleDiff             = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber           = ADC_OFFSET_NONE;
    sConfig.Offset                 = 0;
    sConfig.OffsetSignedSaturation = DISABLE;

    if (HAL_ADC_ConfigChannel(config_.hadc, &sConfig) != HAL_OK) {
        return false;
    }

    uint32_t sum = 0;
    for (int i = 0; i < kNumSamples; ++i) {
        if (HAL_ADC_Start(config_.hadc) != HAL_OK) {
            return false;
        }

        if (HAL_ADC_PollForConversion(config_.hadc, 100) != HAL_OK) {
            HAL_ADC_Stop(config_.hadc);
            return false;
        }

        sum += HAL_ADC_GetValue(config_.hadc);
        HAL_ADC_Stop(config_.hadc);
    }

    out.raw_adc = sum / kNumSamples;

    out.voltage_mv  = calculate_voltage_mv(out.raw_adc);
    out.temperature = calculate_temperature(out.voltage_mv);
    out.valid       = true;

    return out.valid;
}

float LMT85Driver::calculate_voltage_mv(uint32_t raw_adc) const {
    if (config_.adc_max == 0) {
        return 0.0f;
    }
    return config_.adc_vref_mv * static_cast<float>(raw_adc) / static_cast<float>(config_.adc_max);
}

float LMT85Driver::calculate_temperature(float voltage_mv) const {
    // Solve V = v0 + a*(T-t0) + b*(T-t0)^2 for x = (T-t0), taking the root
    // that reduces to x=0 when voltage_mv == v0_mv (TI datasheet Equation 2,
    // rearranged in terms of the generic a/b coefficients above):
    //   x = [-a - sqrt(a^2 - 4*b*(v0 - V))] / (2*b)
    const float a = config_.a_mv_per_c;
    const float b = config_.b_mv_per_c2;

    if (b == 0.0f) {
        // Degenerate to the linear case if the quadratic term is zeroed out.
        if (a == 0.0f) {
            return config_.t0_c;
        }
        return config_.t0_c + (voltage_mv - config_.v0_mv) / a;
    }

    float discriminant = a * a - 4.0f * b * (config_.v0_mv - voltage_mv);
    if (discriminant < 0.0f) {
        discriminant = 0.0f; // out-of-range reading; clamp rather than NaN
    }

    float x = (-a - std::sqrt(discriminant)) / (2.0f * b);
    return config_.t0_c + x;
}

} // namespace LMT85
} // namespace Drivers
