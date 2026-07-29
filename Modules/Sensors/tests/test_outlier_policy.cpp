// multi_sensor_pipeline_tests.cpp
//
// GoogleTest unit tests for multi::Module (as used to build
// PressureEtaSensorModule), covering:
//   - per-sensor raw pressure forwarding
//   - per-sensor error forwarding
//   - the UseOutlier aggregation -> windowed mean -> setter chain
//   - the UseUnpack temperature passthrough
//   - the `valid` mask on the value handed to the return pipeline
//
// -----------------------------------------------------------------------
// ASSUMPTIONS (isolated in the shim section below; adjust if wrong):
//
//  1. `multi::Module`, `multi::PipelineParams`, `multi::UseOutlier`,
//     `multi::UseUnpack`, `multi::SensorParam`/`multi::PressureSensorParam`,
//     `outlier_pipeline::Params<...>`, `average_pipeline::MeanPipeline<...>`,
//     `NoPipeline`, and `multi::PipelineReturnValue` are all assumed to
//     already exist and work exactly as declared/used in the prompt -- none
//     of them are redefined here.
//
//  2. `multi::PressureSensorParam<Sensor, PPipeline>` is assumed to be
//     shorthand for `multi::SensorParam<Sensor, PPipeline, NoPipeline,
//     NoPipeline>` (pressure-only sensor: no temperature pipeline, no
//     per-sensor error pipeline). To be able to assert on a per-sensor
//     error pipeline directly, the tests below use the general
//     `multi::SensorParam<Sensor, PPipeline, TPipeline, ErrorPipeline>`
//     form instead of the `PressureSensorParam` alias. If `SensorParam`
//     isn't directly usable inside `multi::Module`'s sensor-param pack in
//     your real code, swap it back for `PressureSensorParam` and drop the
//     per-sensor error assertions (or however your codebase exposes a
//     pressure-sensor-with-error-pipeline alias).
//
//  3. Just like the single-sensor modules, `multi::Module` is assumed to
//     expose `init()` and `poll()`, and `CommonTimerPolicy` is swapped for
//     `AlwaysPollPolicy` in every test alias so each call to `poll()`
//     deterministically triggers a full read-all-sensors cycle.
//
//  4. Per sensor, on success: `pressure_pipeline.ingest(pressure)` and
//     `temperature_pipeline.ingest(temperature)` are called directly with
//     the raw reading, independent of any cross-sensor treatment. On
//     error: `error.ingest(error_value)` is called instead, and that
//     sensor contributes `valid[i] = false` / an "invalid" candidate to
//     the aggregation step for this cycle.
//
//  5. `UseOutlier<Params, MeanPipeline>` aggregation, per cycle:
//       a. build the candidate set = successful readings whose pressure
//          lies in [Params::min_value, Params::max_value]
//       b. find the subset of size `min(candidates.size(),
//          Params::number_kept)` with minimum variance
//       c. if the resulting kept-subset size >= Params::min_number,
//          forward `mean(kept subset)` into `MeanPipeline::ingest(...)`
//       d. otherwise, nothing is forwarded to `MeanPipeline` this cycle
//          (i.e. `MeanPipeline`'s internal window is only advanced on
//          cycles where the threshold was met -- mirroring the
//          "errors don't consume a window slot" assumption from the
//          single-sensor tests).
//     Test data below always uses two "close together" candidates and one
//     "far away" one, so the minimum-variance pair is unambiguous
//     regardless of the exact tie-breaking rule.
//
//  6. `multi::PipelineReturnValue<UsePressure, UseTemperature,
//     NumberSensors>::valid` is exactly the per-sensor success mask (as
//     given in the prompt), and `::temperature` is exactly
//     `std::array<double, NumberSensors>` of the raw temperature readings
//     for `UseUnpack` (also given in the prompt) -- both are asserted on
//     directly. `::pressure` (an `outlier_pipeline::Frame<NumberSensors>`)
//     has an internal layout that wasn't specified, so its contents are
//     *not* asserted on here; only that the return pipeline is invoked
//     exactly once per poll cycle.
// -----------------------------------------------------------------------

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <deque>
#include <numeric>
#include <vector>

#include "Modules/Sensors/sensor.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"
#include "Modules/Sensors/impl/std/multi.hpp"

// =========================================================================
// Test shims: sensors, error type, and recording policies
// =========================================================================

enum class SensorError {
    HardwareFault,
    Timeout,
    OutOfRange,
};

// A controllable stand-in for SampleSensor, templated on `Tag` so that
// each sensor slot in a multi-sensor module gets its own independent
// static queue (they would otherwise collide on shared static state).
template <typename Tag>
class ControllableSensor {
public:
    using sensor_result = result<pressure_temperature, SensorError>;

    bool init() noexcept {
        ++initCallCount;
        return true;
    }

    sensor_result poll() noexcept {
        ++pollCallCount;
        if (queue.empty()) {
            return sensor_result::success({0.0, 0.0});
        }
        sensor_result next = queue.front();
        queue.pop_front();
        return next;
    }

    static void enqueueSuccess(double pressure, double temperature = 0.0) {
        queue.push_back(sensor_result::success({pressure, temperature}));
    }

    static void enqueueError(SensorError err) {
        queue.push_back(sensor_result::error(err));
    }

    static void reset() {
        queue.clear();
        initCallCount = 0;
        pollCallCount = 0;
    }

    static inline std::deque<sensor_result> queue;
    static inline int initCallCount = 0;
    static inline int pollCallCount = 0;
};

// Recording setter policy / pipeline stage (per-sensor raw setter, or the
// aggregate mean setter). `Tag` forces distinct static storage per slot.
template <typename Tag>
struct RecordingSink {
    inline void ingest(const auto &value) noexcept {
        values.push_back(static_cast<double>(value));
    }

    static void reset() { values.clear(); }

    static inline std::vector<double> values;
};

// Recording per-sensor error pipeline.
template <typename Tag>
struct RecordingErrorPipeline {
    inline void ingest(const auto &error) noexcept { errors.push_back(error); }

    static void reset() { errors.clear(); }

    static inline std::vector<SensorError> errors;
};

// Recording return pipeline: stores every `PipelineReturnValue` frame it's
// given, verbatim, so tests can inspect the fully-specified fields
// (`valid`, `temperature`) without needing to know the layout of the
// UseOutlier-specific `pressure` field.
template <typename Tag, typename Frame>
struct RecordingReturnPipeline {
    inline void ingest(const Frame &frame) noexcept { frames.push_back(frame); }

    static void reset() { frames.clear(); }

    static inline std::vector<Frame> frames;
};

namespace tags {
struct Sensor1 {};
struct Sensor2 {};
struct Sensor3 {};

struct Pressure1 {};
struct Pressure2 {};
struct Pressure3 {};

struct Error1 {};
struct Error2 {};
struct Error3 {};

struct PressureMean {};
struct ReturnValue {};
}  // namespace tags

// =========================================================================
// Pipeline configuration under test
// =========================================================================

constexpr std::size_t kEtaNumberKept = 2;
constexpr std::size_t kEtaMinNumber = 1;
constexpr double kEtaMinValue = 0.0;
constexpr double kEtaMaxValue = 100.0;
constexpr std::size_t kEtaWindowSize = 3;
constexpr std::size_t kNumberSensors = 3;

using Sensor1 = ControllableSensor<tags::Sensor1>;
using Sensor2 = ControllableSensor<tags::Sensor2>;
using Sensor3 = ControllableSensor<tags::Sensor3>;

using Pressure1Sink = RecordingSink<tags::Pressure1>;
using Pressure2Sink = RecordingSink<tags::Pressure2>;
using Pressure3Sink = RecordingSink<tags::Pressure3>;

using Error1Sink = RecordingErrorPipeline<tags::Error1>;
using Error2Sink = RecordingErrorPipeline<tags::Error2>;
using Error3Sink = RecordingErrorPipeline<tags::Error3>;

using PressureMeanSink = RecordingSink<tags::PressureMean>;

using PressureUse = multi::UseOutlier<
    outlier_pipeline::Params<kEtaNumberKept, kEtaMinNumber, kEtaMinValue, kEtaMaxValue>,
    average_pipeline::MeanPipeline<kEtaWindowSize, PressureMeanSink>>;

using ReturnFrame = multi::internal::PipelineReturnValue<PressureUse, multi::UseUnpack, kNumberSensors>;
using ReturnSink = RecordingReturnPipeline<tags::ReturnValue, ReturnFrame>;

using TestPressureEtaSensorModule = multi::Module<
    AlwaysPollPolicy,
    multi::PipelineParams<
        PressureUse,
        multi::UseUnpack,
        ReturnSink>,

    multi::SensorParam<Sensor1, Pressure1Sink, NoPipeline, Error1Sink>,
    multi::SensorParam<Sensor2, Pressure2Sink, NoPipeline, Error2Sink>,
    multi::SensorParam<Sensor3, Pressure3Sink, NoPipeline, Error3Sink>>;

// =========================================================================
// Helpers
// =========================================================================

namespace {

double MeanOfLastN(const std::vector<double> &samples, std::size_t n) {
    EXPECT_GE(samples.size(), n);
    const std::size_t start = samples.size() - n;
    return std::accumulate(samples.begin() + start, samples.end(), 0.0) / static_cast<double>(n);
}

}  // namespace

class PressureEtaSensorModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        Sensor1::reset();
        Sensor2::reset();
        Sensor3::reset();
        Pressure1Sink::reset();
        Pressure2Sink::reset();
        Pressure3Sink::reset();
        Error1Sink::reset();
        Error2Sink::reset();
        Error3Sink::reset();
        PressureMeanSink::reset();
        ReturnSink::reset();
    }
};

// -------------------------------------------------------------------------
// Per-sensor raw forwarding (independent of the outlier treatment)
// -------------------------------------------------------------------------

TEST_F(PressureEtaSensorModuleTest, EachSensorsSuccessGoesToItsOwnRawSetter) {
    Sensor1::enqueueSuccess(10.0);
    Sensor2::enqueueSuccess(20.0);
    Sensor3::enqueueSuccess(30.0);

    TestPressureEtaSensorModule module;
    module.init();
    module.tick();

    ASSERT_EQ(Pressure1Sink::values.size(), 1u);
    EXPECT_DOUBLE_EQ(Pressure1Sink::values.front(), 10.0);

    ASSERT_EQ(Pressure2Sink::values.size(), 1u);
    EXPECT_DOUBLE_EQ(Pressure2Sink::values.front(), 20.0);

    ASSERT_EQ(Pressure3Sink::values.size(), 1u);
    EXPECT_DOUBLE_EQ(Pressure3Sink::values.front(), 30.0);

    EXPECT_TRUE(Error1Sink::errors.empty());
    EXPECT_TRUE(Error2Sink::errors.empty());
    EXPECT_TRUE(Error3Sink::errors.empty());
}

TEST_F(PressureEtaSensorModuleTest, EachSensorsErrorGoesToItsOwnErrorPipelineOnly) {
    Sensor1::enqueueError(SensorError::HardwareFault);
    Sensor2::enqueueSuccess(20.0);
    Sensor3::enqueueError(SensorError::Timeout);

    TestPressureEtaSensorModule module;
    module.init();
    module.tick();

    ASSERT_EQ(Error1Sink::errors.size(), 1u);
    EXPECT_EQ(Error1Sink::errors.front(), SensorError::HardwareFault);
    EXPECT_TRUE(Pressure1Sink::values.empty());

    EXPECT_TRUE(Error2Sink::errors.empty());
    ASSERT_EQ(Pressure2Sink::values.size(), 1u);
    EXPECT_DOUBLE_EQ(Pressure2Sink::values.front(), 20.0);

    ASSERT_EQ(Error3Sink::errors.size(), 1u);
    EXPECT_EQ(Error3Sink::errors.front(), SensorError::Timeout);
    EXPECT_TRUE(Pressure3Sink::values.empty());
}

// -------------------------------------------------------------------------
// UseOutlier aggregation -> windowed mean -> setter
// -------------------------------------------------------------------------

TEST_F(PressureEtaSensorModuleTest, OutlierMeanIsForwardedOnceWindowFills) {
    // Each cycle: two close-together sensors + one far outlier.
    // kEtaNumberKept == 2, kEtaMinNumber == 2, kEtaWindowSize == 3.
    Sensor1::enqueueSuccess(10.0);
    Sensor2::enqueueSuccess(10.2);
    Sensor3::enqueueSuccess(90.0);  // outlier, excluded from the kept pair

    Sensor1::enqueueSuccess(20.0);
    Sensor2::enqueueSuccess(20.1);
    Sensor3::enqueueSuccess(95.0);

    Sensor1::enqueueSuccess(30.0);
    Sensor2::enqueueSuccess(29.9);
    Sensor3::enqueueSuccess(85.0);

    TestPressureEtaSensorModule module;
    module.init();
    module.tick();  // cycle 1: kept mean = mean(10.0, 10.2)   = 10.10
    module.tick();  // cycle 2: kept mean = mean(20.0, 20.1)   = 20.05
    module.tick();  // cycle 3: kept mean = mean(30.0, 29.9)   = 29.95 -> window full

    const std::vector<double> cycleMeans = {10.10, 20.05, 29.95};
    ASSERT_EQ(PressureMeanSink::values.size(), 1u);
    EXPECT_NEAR(PressureMeanSink::values.front(), MeanOfLastN(cycleMeans, kEtaWindowSize), 1e-9);
}

TEST_F(PressureEtaSensorModuleTest, OutlierMeanSlidesAsNewCyclesArrive) {
    Sensor1::enqueueSuccess(10.0);
    Sensor2::enqueueSuccess(10.2);
    Sensor3::enqueueSuccess(90.0);

    Sensor1::enqueueSuccess(20.0);
    Sensor2::enqueueSuccess(20.1);
    Sensor3::enqueueSuccess(95.0);

    Sensor1::enqueueSuccess(30.0);
    Sensor2::enqueueSuccess(29.9);
    Sensor3::enqueueSuccess(85.0);

    Sensor1::enqueueSuccess(60.0);
    Sensor2::enqueueSuccess(59.8);
    Sensor3::enqueueSuccess(5.0);

    TestPressureEtaSensorModule module;
    module.init();
    for (int i = 0; i < 4; ++i) module.tick();

    const std::vector<double> cycleMeans = {10.10, 20.05, 29.95, 59.90};
    ASSERT_EQ(PressureMeanSink::values.size(), 2u);
    EXPECT_NEAR(PressureMeanSink::values[0],
                MeanOfLastN({cycleMeans[0], cycleMeans[1], cycleMeans[2]}, kEtaWindowSize), 1e-9);
    EXPECT_NEAR(PressureMeanSink::values[1],
                MeanOfLastN({cycleMeans[1], cycleMeans[2], cycleMeans[3]}, kEtaWindowSize), 1e-9);
}

TEST_F(PressureEtaSensorModuleTest, TooFewValidSamplesDoesNotAdvanceTheWindow) {
    // Two sensors error out; only one valid reading remains, which is below
    // kEtaMinNumber (2), so nothing should be forwarded to the mean sink.
    Sensor1::enqueueError(SensorError::HardwareFault);
    Sensor2::enqueueError(SensorError::Timeout);
    Sensor3::enqueueSuccess(42.0);

    TestPressureEtaSensorModule module;
    module.init();
    module.tick();

    EXPECT_TRUE(PressureMeanSink::values.empty());
    // The lone successful sensor still reports its raw value regardless.
    ASSERT_EQ(Pressure3Sink::values.size(), 1u);
    EXPECT_DOUBLE_EQ(Pressure3Sink::values.front(), 42.0);
}

TEST_F(PressureEtaSensorModuleTest, OutOfRangeValueIsExcludedFromOutlierButStillReachesRawSetter) {
    Sensor1::enqueueSuccess(11.0);
    Sensor2::enqueueSuccess(13.0);
    Sensor3::enqueueSuccess(500.0);  // outside [0, 100]
    Sensor1::enqueueSuccess(10.9);
    Sensor2::enqueueSuccess(13.0);
    Sensor3::enqueueSuccess(-12.0); 
    Sensor1::enqueueError(SensorError::HardwareFault);
    Sensor2::enqueueSuccess(12.05);
    Sensor3::enqueueError(SensorError::Timeout);
    Sensor1::enqueueError(SensorError::HardwareFault);
    Sensor2::enqueueError(SensorError::Timeout);
    Sensor3::enqueueError(SensorError::Timeout);

    TestPressureEtaSensorModule module;
    module.init();
    module.tick();
    module.tick();
    module.tick();

    ASSERT_EQ(PressureMeanSink::values.size(), 1u);
    EXPECT_NEAR(PressureMeanSink::values.front(), (11.0 + 13.0) / 2.0, 1e-9);

    ASSERT_EQ(Pressure3Sink::values.size(), 2u);
    EXPECT_DOUBLE_EQ(Pressure3Sink::values.front(), 500.0);
}

// -------------------------------------------------------------------------
// Return pipeline: `valid` mask and UseUnpack temperature passthrough
// -------------------------------------------------------------------------

TEST_F(PressureEtaSensorModuleTest, ReturnPipelineReceivesValidMaskMatchingSensorOutcomes) {
    Sensor1::enqueueSuccess(10.0, /*temperature=*/1.0);
    Sensor2::enqueueError(SensorError::Timeout);
    Sensor3::enqueueSuccess(30.0, /*temperature=*/3.0);

    TestPressureEtaSensorModule module;
    module.init();
    module.tick();

    ASSERT_EQ(ReturnSink::frames.size(), 1u);
    const auto &frame = ReturnSink::frames.front();

    ASSERT_EQ(frame.valid.size(), kNumberSensors);
    EXPECT_TRUE(frame.valid[0]);
    EXPECT_FALSE(frame.valid[1]);
    EXPECT_TRUE(frame.valid[2]);

    // UseUnpack: raw, per-sensor temperature readings, unpacked as-is.
    ASSERT_EQ(frame.temperature.size(), kNumberSensors);
    EXPECT_DOUBLE_EQ(frame.temperature[0], 1.0);
    EXPECT_DOUBLE_EQ(frame.temperature[2], 3.0);
    // frame.temperature[1] is not asserted on: its value for a failed
    // sensor slot wasn't specified (could be 0.0, NaN, or last-known-value
    // depending on your implementation).
}

TEST_F(PressureEtaSensorModuleTest, ReturnPipelineIsInvokedExactlyOncePerPoll) {
    Sensor1::enqueueSuccess(1.0);
    Sensor2::enqueueSuccess(2.0);
    Sensor3::enqueueSuccess(3.0);

    Sensor1::enqueueSuccess(4.0);
    Sensor2::enqueueSuccess(5.0);
    Sensor3::enqueueSuccess(6.0);

    TestPressureEtaSensorModule module;
    module.init();
    module.tick();
    module.tick();

    EXPECT_EQ(ReturnSink::frames.size(), 2u);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}