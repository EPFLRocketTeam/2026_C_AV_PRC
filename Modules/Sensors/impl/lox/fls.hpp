
#pragma once
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/lox/consts.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"
#include "cstdio"

const char fls_error_msg[] = "Could Not Read FLS\r\n";

template<const char* str>
struct SimpleErrorPipeline {
	void ingest (const auto &err) {
		// Silenced -- FLS is unwired on the bench, spams this every poll.
		// Re-enable if you need to see it again.
		// printf(str);
		(void)err;
	}
};


using FLSModule = SensorModule<
    CommonTimerPolicy,
    SampleDoubleSensor,
    IfPipeline<
        average_pipeline::SimplePipeline<
            LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_FLS),
            FLS_WINDOW_SIZE,
            LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_FLS_mean)
        >,
		SimpleErrorPipeline<fls_error_msg>
    >
>;
