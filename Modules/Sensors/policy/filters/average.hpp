
#pragma once
#include "sigutils/average.hpp"

template<
    const int WindowSize,
    typename FloatType,
    typename ForwardType
>
struct _RunningAveragePolicy {
private:
    RunningAverage<FloatType, WindowSize> runningAverage;

    ForwardType forward;
public:
    void set (const FloatType &data) {
        runningAverage.push(data);

        if (runningAverage.ready()) {
            forward.set(runningAverage.mean());
        }
    }
};

template<
    const int WindowSize,
    typename ForwardType
>
using RunningAveragePolicy = _RunningAveragePolicy<WindowSize, double, ForwardType>;
