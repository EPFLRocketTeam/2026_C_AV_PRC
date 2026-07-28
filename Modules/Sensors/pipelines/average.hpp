
/**
 * This simple pipeline takes a single sensor that outputs pressure or/and temperature and uses it to
 * produce a raw pressure or/and temperature and output a mean pressure or/and temperature
 */
#include "sigutils/average.hpp"
#include "Modules/Sensors/pipelines/nothing.hpp"

namespace average_pipeline {

    template<typename Setter, const size_t WindowSize, typename MeanSetter>
    struct SimplePipeline {
    private:
        Setter     setter;
        MeanSetter meanSetter;

        RunningAverage<double, WindowSize> runningAverage;
    public:
        void ingest (const double &value) {
            setter.ingest(value);

            runningAverage.push(value);
            if (runningAverage.ready()) {
                meanSetter.ingest(runningAverage.mean());
            }
        }
    };
    
    template<const size_t WindowSize, typename MeanSetter>
    using MeanPipeline = SimplePipeline<NoPipeline, WindowSize, MeanSetter>;

};
