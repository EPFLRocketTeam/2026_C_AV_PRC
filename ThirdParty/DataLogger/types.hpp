#pragma once
#include <stddef.h>
#include "Application/Data/data.hpp"
#include "Application/Data/fsm.hpp"
#include "Application/Data/engine_fsm.hpp"
#include "Modules/Sensors/impl/std/multi.hpp"
#include "Drivers/Plume/types.hpp"

constexpr uint8_t ENGINE_LOGGER_MAGIC = 0xA4;
constexpr uint8_t LOX_LOGGER_MAGIC = 0xC1;
constexpr uint8_t ETH_LOGGER_MAGIC = 0x8B;

struct LogHeader {
    uint8_t  magic;
    uint8_t  record_type;
    uint16_t length;
    uint32_t timestamp_us;
};

struct BaseStorageHealth {
    uint32_t bytes_written_ = 0;
    uint32_t write_count_ = 0;
    uint32_t write_fail_count_ = 0;
    uint32_t max_write_time_us_ = 0;
    uint32_t tick_count_ = 0;
};

struct StorageHealth {
    BaseStorageHealth health;
    SdTimingStats timing;

    size_t disk_size_remaining;
    size_t arena_used_bytes;
    size_t arena_total_bytes;
};

struct pressures_frame {
    double pressure;
    double pressure_mean;
};
struct temperature_frame {
    double temperature;
    double temperature_mean;
};
struct pressure_temperature_frame {
    double pressure;
    double pressure_mean;
    
    double temperature;
    double temperature_mean;
};

struct engine_fsm_transition {
    prc::EngineState old_state;
    prc::EngineState new_state;
};

struct dpr_fsm_transition {
    prc::State old_state;
    prc::State new_state;
};

namespace engine {

    enum ErrorKind {
        CHAMBER_ERROR,
        
        EIN_P_ERROR,
        EIN_T_ERROR,

        OIN_P_ERROR,
        OIN_T_ERROR
    };

    enum RecordType {
        LOG_HEALTH,

        LOG_DATA_DUMP,
        LOG_FSM_TRANSITION,

        LOG_CHAMBER_FRAME, // { P, T, P_mean, C_mean }

        LOG_EIN_P_FRAME, // { P, P_mean }
        LOG_EIN_T_FRAME, // { T, T_mean }
        LOG_OIN_P_FRAME, // { P, P_mean }
        LOG_OIN_T_FRAME, // { T, T_mean }

        LOG_ERROR // Send an EngineErrorKind
    };
};

namespace lox {

    // For now, no OTA2 is present
    using OtaPressureFrame = multi::internal::PipelineReturnValue<
        multi::UseOutlier<int, int>,
        multi::UseUnpack,
        2
    >;

    struct OtaTemperatureFrame {
        uint8_t sensor_id;
        
        double temperature;
        double temperature_mean;
    };

    enum ErrorKind {
        FLS_ERROR,
        
        HPO_P_ERROR,
        OTA_P_ERROR,

        OTA1_P_ERROR,
        OTA2_P_ERROR,
        OTA3_P_ERROR,

        OTA1_T_ERROR,
        OTA2_T_ERROR,
        OTA3_T_ERROR,
        OTA4_T_ERROR
    };

    enum RecordType {
        LOG_HEALTH,
        LOG_DATA_DUMP,
        LOG_FSM_TRANSITION,

        LOG_FLS, // fill level

        LOG_HPO_FRAME, // { P, P_mean }
        LOG_OTA_P_FRAME, // OtaPressureFrame
        LOG_OTA_T_FRAME, // OtaTemperatureFrame

        LOG_ERROR // Send a LoxErrorKind
    };

}; // namespace lox;



namespace eth {

    // For now, no OTA3 is present
    using EtaPressureFrame = multi::internal::PipelineReturnValue<
        multi::UseOutlier<int, int>,
        multi::UseUnpack,
        2
    >;

    enum ErrorKind {
        FLS_ERROR,
        
        HPE_P_ERROR,
        ETA_P_ERROR,

        ETA1_P_ERROR,
        ETA2_P_ERROR,
        ETA3_P_ERROR,
    };

    enum RecordType {
        LOG_HEALTH,

        LOG_DATA_DUMP,
        LOG_FSM_TRANSITION,

        LOG_HPE_FRAME, // { P, P_mean }
        LOG_ETA_P_FRAME, // OtaPressureFrame
        
        LOG_ERROR // Send a LoxErrorKind
    };
};
