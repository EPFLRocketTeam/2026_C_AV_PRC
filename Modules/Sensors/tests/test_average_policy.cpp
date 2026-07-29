// sensor_pipeline_tests.cpp
//
// GoogleTest unit tests for PressureModule / TemperatureModule / BothModule.
//
// -----------------------------------------------------------------------
// ASSUMPTIONS (please adjust the small "shim" section below if these are
// wrong -- everything that depends on them is isolated there):
//
//  1. `result<T, E>` is a Rust-like result type constructed via the static
//     factories shown in your snippet: `result<T,E>::success(T)` and
//     `result<T,E>::error(E)`. Nothing else about its API is used here.
//
//  2. A "sensor" is default constructible, has `bool init()` and
//     `sensor_result poll()`, both non-static (as shown by SampleSensor).
//     For the tests we use a controllable stand-in, `ControllableSensor`,
//     that queues up canned results to return from `poll()`. It shares
//     the same *shape* as SampleSensor so it can be dropped into the
//     Sensor slot of PressureModule/TemperatureModule/BothModule.
//
//  3. A "setter policy" / "pipeline" (the things produced by
//     ETH_SETTER_POLICY(...)/ENGINE_SETTER_POLICY(...) and the ErrorPipeline
//     itself) all conform to the same tiny concept shown by `NoPipeline`:
//
//         void ingest(const auto &data);
//
//     So a recording test double only needs to implement `ingest` and
//     stash the value somewhere inspectable (a static vector, since the
//     Module is expected to default-construct its policy members and we
//     have no handle to the instance it creates internally).
//
//  4. The sensor's error type is a placeholder `SensorError` enum (the
//     `???` in your snippet) -- swap this for whatever concrete error type
//     your real sensors use; the tests don't care about its value, only
//     that it round-trips through the ErrorPipeline unchanged.
//
//  5. `CommonTimerPolicy` is replaced by `AlwaysPollPolicy` (given in your
//     snippet) in every test alias below, so that every call the test makes
//     to trigger a cycle actually results in a poll -- this keeps tests
//     deterministic and removes real-time dependence.
//
//  6. The module exposes `init()` (calls through to the sensor's init) and
//     `poll()` (runs exactly one "should I poll -> poll -> route result"
//     cycle). If your real class names these differently (e.g. `update()`,
//     `tick()`, `run()`), rename the two call sites -- search for
//     `.init()` / `.poll()` on the module (not the sensor) in this file.
//
//  7. On error, PressureModule/TemperatureModule/BothModule call
//     `ErrorPipeline::ingest(error)` and do NOT touch the value setter,
//     the window, or the mean setter for that cycle. BothModule calls the
//     ErrorPipeline exactly once per failed poll (not once per field).
//
//  8. Failed polls do not consume a "slot" in the window -- i.e. the mean
//     is computed from the last WindowSize *successful* samples. If your
//     implementation instead counts every poll (success or not) towards
//     the window, see `ErrorsDoNotCountTowardWindow` below and adjust.
// -----------------------------------------------------------------------

#include <gtest/gtest.h>

#include <cstddef>
#include <deque>
#include <numeric>
#include <vector>

#include "Modules/Sensors/sensor.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"

// Real pipeline headers (result<>, PressureModule, TemperatureModule,
// BothModule, NoPipeline, AlwaysPollPolicy, etc.) are assumed to be
// included by the build already, as requested.

// =========================================================================
// Test shims: sensor, error type, and recording policies
// =========================================================================

// Placeholder for the sensor error type (`???` in the prompt).
enum class SensorError {
    HardwareFault,
    Timeout,
    OutOfRange,
};

// A controllable stand-in for SampleSensor. Shares SampleSensor's shape
// (default constructible, `init()`, `poll()`) but returns canned results
// from a queue instead of `rand()`, so tests are deterministic.
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

    // --- test control surface (static: the Module owns its own instance,
    //     so tests reach in via shared class-level state) ---

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

// A recording "setter policy" / pipeline stage. `Tag` is only used to force
// a distinct type (and therefore distinct static storage) per slot, so
// e.g. the pressure setter and the pressure-mean setter don't share a bin.
template <typename Tag>
struct RecordingSink {
    inline void ingest(const auto &value) noexcept {
        values.push_back(static_cast<double>(value));
    }

    static void reset() { values.clear(); }

    static inline std::vector<double> values;
};

// A recording ErrorPipeline stand-in.
template <typename Tag>
struct RecordingErrorPipeline {
    inline void ingest(const auto &error) noexcept { errors.push_back(error); }

    static void reset() { errors.clear(); }

    static inline std::vector<SensorError> errors;
};

namespace tags {
struct PressureValue {};
struct PressureMean {};
struct PressureError {};

struct TemperatureValue {};
struct TemperatureMean {};
struct TemperatureError {};

struct BothPressureValue {};
struct BothPressureMean {};
struct BothTemperatureValue {};
struct BothTemperatureMean {};
struct BothError {};
}  // namespace tags

constexpr std::size_t kPressureWindowSize = 4;
constexpr std::size_t kTemperatureWindowSize = 3;
constexpr std::size_t kBothPressureWindowSize = 4;
constexpr std::size_t kBothTemperatureWindowSize = 2;

using PressureValueSink = RecordingSink<tags::PressureValue>;
using PressureMeanSink = RecordingSink<tags::PressureMean>;
using PressureErrorSink = RecordingErrorPipeline<tags::PressureError>;

using TemperatureValueSink = RecordingSink<tags::TemperatureValue>;
using TemperatureMeanSink = RecordingSink<tags::TemperatureMean>;
using TemperatureErrorSink = RecordingErrorPipeline<tags::TemperatureError>;

using BothPressureValueSink = RecordingSink<tags::BothPressureValue>;
using BothPressureMeanSink = RecordingSink<tags::BothPressureMean>;
using BothTemperatureValueSink = RecordingSink<tags::BothTemperatureValue>;
using BothTemperatureMeanSink = RecordingSink<tags::BothTemperatureMean>;
using BothErrorSink = RecordingErrorPipeline<tags::BothError>;

// =========================================================================
// Module aliases under test
// =========================================================================

using TestPressureModule = PressureModule<
    AlwaysPollPolicy,
    ControllableSensor,
    PressureValueSink,
    kPressureWindowSize,
    PressureMeanSink,
    PressureErrorSink>;

using TestTemperatureModule = TemperatureModule<
    AlwaysPollPolicy,
    ControllableSensor,
    TemperatureValueSink,
    kTemperatureWindowSize,
    TemperatureMeanSink,
    TemperatureErrorSink>;

using TestBothModule = BothModule<
    AlwaysPollPolicy,
    ControllableSensor,
    BothPressureValueSink,
    kBothPressureWindowSize,
    BothPressureMeanSink,
    BothTemperatureValueSink,
    kBothTemperatureWindowSize,
    BothTemperatureMeanSink,
    BothErrorSink>;

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

// =========================================================================
// PressureModule
// =========================================================================

class PressureModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        ControllableSensor::reset();
        PressureValueSink::reset();
        PressureMeanSink::reset();
        PressureErrorSink::reset();
    }
};

TEST_F(PressureModuleTest, ErrorIsForwardedToErrorPipelineOnly) {
    ControllableSensor::enqueueError(SensorError::HardwareFault);

    TestPressureModule module;
    module.init();
    module.tick();

    ASSERT_EQ(PressureErrorSink::errors.size(), 1u);
    EXPECT_EQ(PressureErrorSink::errors.front(), SensorError::HardwareFault);
    EXPECT_TRUE(PressureValueSink::values.empty());
    EXPECT_TRUE(PressureMeanSink::values.empty());
}

TEST_F(PressureModuleTest, SuccessValueIsForwardedToSetter) {
    ControllableSensor::enqueueSuccess(/*pressure=*/12.5);

    TestPressureModule module;
    module.init();
    module.tick();

    ASSERT_EQ(PressureValueSink::values.size(), 1u);
    EXPECT_DOUBLE_EQ(PressureValueSink::values.front(), 12.5);
    EXPECT_TRUE(PressureMeanSink::values.empty());  // window not full yet
    EXPECT_TRUE(PressureErrorSink::errors.empty());
}

TEST_F(PressureModuleTest, MeanIsComputedOnceWindowFills) {
    const std::vector<double> samples = {1.0, 2.0, 3.0, 4.0};
    for (double s : samples) ControllableSensor::enqueueSuccess(s);

    TestPressureModule module;
    module.init();
    for (std::size_t i = 0; i < samples.size(); ++i) module.tick();

    ASSERT_EQ(PressureValueSink::values.size(), samples.size());
    ASSERT_EQ(PressureMeanSink::values.size(), 1u);
    EXPECT_DOUBLE_EQ(PressureMeanSink::values.front(), 2.5);  // mean(1,2,3,4)
}

TEST_F(PressureModuleTest, MeanSlidesAsNewSamplesArrive) {
    const std::vector<double> samples = {1.0, 2.0, 3.0, 4.0, 8.0};
    for (double s : samples) ControllableSensor::enqueueSuccess(s);

    TestPressureModule module;
    module.init();
    for (std::size_t i = 0; i < samples.size(); ++i) module.tick();

    ASSERT_EQ(PressureMeanSink::values.size(), 2u);
    EXPECT_DOUBLE_EQ(PressureMeanSink::values[0], MeanOfLastN({1.0, 2.0, 3.0, 4.0}, kPressureWindowSize));
    EXPECT_DOUBLE_EQ(PressureMeanSink::values[1], MeanOfLastN({1.0, 2.0, 3.0, 4.0, 8.0}, kPressureWindowSize));
}

TEST_F(PressureModuleTest, ErrorsDoNotCountTowardWindow) {
    ControllableSensor::enqueueSuccess(1.0);
    ControllableSensor::enqueueSuccess(2.0);
    ControllableSensor::enqueueError(SensorError::Timeout);
    ControllableSensor::enqueueSuccess(3.0);
    ControllableSensor::enqueueSuccess(4.0);

    TestPressureModule module;
    module.init();
    for (int i = 0; i < 5; ++i) module.tick();

    EXPECT_EQ(PressureErrorSink::errors.size(), 1u);
    ASSERT_EQ(PressureValueSink::values.size(), 4u);
    ASSERT_EQ(PressureMeanSink::values.size(), 1u);
    EXPECT_DOUBLE_EQ(PressureMeanSink::values.front(), 2.5);  // mean(1,2,3,4)
}

// =========================================================================
// TemperatureModule
// =========================================================================

class TemperatureModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        ControllableSensor::reset();
        TemperatureValueSink::reset();
        TemperatureMeanSink::reset();
        TemperatureErrorSink::reset();
    }
};

TEST_F(TemperatureModuleTest, ErrorIsForwardedToErrorPipelineOnly) {
    ControllableSensor::enqueueError(SensorError::OutOfRange);

    TestTemperatureModule module;
    module.init();
    module.tick();

    ASSERT_EQ(TemperatureErrorSink::errors.size(), 1u);
    EXPECT_EQ(TemperatureErrorSink::errors.front(), SensorError::OutOfRange);
    EXPECT_TRUE(TemperatureValueSink::values.empty());
    EXPECT_TRUE(TemperatureMeanSink::values.empty());
}

TEST_F(TemperatureModuleTest, SuccessValueIsForwardedToSetter) {
    ControllableSensor::enqueueSuccess(/*pressure=*/0.0, /*temperature=*/36.6);

    TestTemperatureModule module;
    module.init();
    module.tick();

    ASSERT_EQ(TemperatureValueSink::values.size(), 1u);
    EXPECT_DOUBLE_EQ(TemperatureValueSink::values.front(), 36.6);
    EXPECT_TRUE(TemperatureMeanSink::values.empty());
    EXPECT_TRUE(TemperatureErrorSink::errors.empty());
}

TEST_F(TemperatureModuleTest, MeanIsComputedOnceWindowFills) {
    const std::vector<double> samples = {10.0, 20.0, 30.0};  // kTemperatureWindowSize == 3
    for (double s : samples) ControllableSensor::enqueueSuccess(/*pressure=*/0.0, s);

    TestTemperatureModule module;
    module.init();
    for (std::size_t i = 0; i < samples.size(); ++i) module.tick();

    ASSERT_EQ(TemperatureValueSink::values.size(), samples.size());
    ASSERT_EQ(TemperatureMeanSink::values.size(), 1u);
    EXPECT_DOUBLE_EQ(TemperatureMeanSink::values.front(), 20.0);
}

TEST_F(TemperatureModuleTest, MeanSlidesAsNewSamplesArrive) {
    const std::vector<double> samples = {10.0, 20.0, 30.0, 60.0};
    for (double s : samples) ControllableSensor::enqueueSuccess(/*pressure=*/0.0, s);

    TestTemperatureModule module;
    module.init();
    for (std::size_t i = 0; i < samples.size(); ++i) module.tick();

    ASSERT_EQ(TemperatureMeanSink::values.size(), 2u);
    EXPECT_DOUBLE_EQ(TemperatureMeanSink::values[0], MeanOfLastN({10.0, 20.0, 30.0}, kTemperatureWindowSize));
    EXPECT_DOUBLE_EQ(TemperatureMeanSink::values[1], MeanOfLastN({10.0, 20.0, 30.0, 60.0}, kTemperatureWindowSize));
}

// =========================================================================
// BothModule
// =========================================================================

class BothModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        ControllableSensor::reset();
        BothPressureValueSink::reset();
        BothPressureMeanSink::reset();
        BothTemperatureValueSink::reset();
        BothTemperatureMeanSink::reset();
        BothErrorSink::reset();
    }
};

TEST_F(BothModuleTest, ErrorIsForwardedOnceAndNeitherSetterIsCalled) {
    ControllableSensor::enqueueError(SensorError::HardwareFault);

    TestBothModule module;
    module.init();
    module.tick();

    ASSERT_EQ(BothErrorSink::errors.size(), 1u);
    EXPECT_EQ(BothErrorSink::errors.front(), SensorError::HardwareFault);
    EXPECT_TRUE(BothPressureValueSink::values.empty());
    EXPECT_TRUE(BothPressureMeanSink::values.empty());
    EXPECT_TRUE(BothTemperatureValueSink::values.empty());
    EXPECT_TRUE(BothTemperatureMeanSink::values.empty());
}

TEST_F(BothModuleTest, SuccessForwardsBothFieldsToTheirOwnSetters) {
    ControllableSensor::enqueueSuccess(/*pressure=*/5.0, /*temperature=*/42.0);

    TestBothModule module;
    module.init();
    module.tick();

    ASSERT_EQ(BothPressureValueSink::values.size(), 1u);
    EXPECT_DOUBLE_EQ(BothPressureValueSink::values.front(), 5.0);

    ASSERT_EQ(BothTemperatureValueSink::values.size(), 1u);
    EXPECT_DOUBLE_EQ(BothTemperatureValueSink::values.front(), 42.0);

    EXPECT_TRUE(BothErrorSink::errors.empty());
}

TEST_F(BothModuleTest, PressureAndTemperatureMeansAreComputedIndependently) {
    // kBothPressureWindowSize == 4, kBothTemperatureWindowSize == 2, so the
    // temperature mean fires twice by the time pressure fires once.
    const std::vector<std::pair<double, double>> samples = {
        {1.0, 100.0},
        {2.0, 200.0},
        {3.0, 300.0},
        {4.0, 400.0},
    };
    for (auto [p, t] : samples) ControllableSensor::enqueueSuccess(p, t);

    TestBothModule module;
    module.init();
    for (std::size_t i = 0; i < samples.size(); ++i) module.tick();

    ASSERT_EQ(BothPressureMeanSink::values.size(), 1u);
    EXPECT_DOUBLE_EQ(BothPressureMeanSink::values.front(), 2.5);  // mean(1,2,3,4)

    ASSERT_EQ(BothTemperatureMeanSink::values.size(), 3u);
    EXPECT_DOUBLE_EQ(BothTemperatureMeanSink::values[0], 150.0);  // mean(100,200)
    EXPECT_DOUBLE_EQ(BothTemperatureMeanSink::values[1], 250.0);  // mean(300,400)
    EXPECT_DOUBLE_EQ(BothTemperatureMeanSink::values[2], 350.0);  // mean(300,400)
}

TEST_F(BothModuleTest, ErrorsDoNotCountTowardEitherWindow) {
    ControllableSensor::enqueueSuccess(1.0, 100.0);
    ControllableSensor::enqueueError(SensorError::Timeout);
    ControllableSensor::enqueueSuccess(2.0, 200.0);

    TestBothModule module;
    module.init();
    for (int i = 0; i < 3; ++i) module.tick();

    EXPECT_EQ(BothErrorSink::errors.size(), 1u);
    ASSERT_EQ(BothPressureValueSink::values.size(), 2u);
    ASSERT_EQ(BothTemperatureValueSink::values.size(), 2u);

    // kBothTemperatureWindowSize == 2, so the mean should have fired exactly
    // once, over the two *successful* temperature samples.
    ASSERT_EQ(BothTemperatureMeanSink::values.size(), 1u);
    EXPECT_DOUBLE_EQ(BothTemperatureMeanSink::values.front(), 150.0);  // mean(100,200)

    // kBothPressureWindowSize == 4, only 2 successful samples so far.
    EXPECT_TRUE(BothPressureMeanSink::values.empty());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}