#pragma once
#include <stddef.h>
#include "Application/Data/data.hpp"
#include "Application/Data/fsm.hpp"
#include "Application/Data/engine_fsm.hpp"
#include "Modules/Sensors/impl/std/multi.hpp"
#include "Drivers/Plume/types.hpp"
#include "Drivers/SensataPte7300/Types.hpp"

constexpr uint8_t ENGINE_LOGGER_MAGIC = 0xA4;
constexpr uint8_t LOX_LOGGER_MAGIC = 0xC1;
constexpr uint8_t ETH_LOGGER_MAGIC = 0x8B;

struct LogHeader {
    uint8_t  magic;
    uint8_t  record_type;
    uint16_t length;
    uint32_t timestamp_us;
};
static_assert(sizeof(LogHeader) == 8);

struct BaseStorageHealth {
    uint32_t bytes_written_ = 0;
    uint32_t write_count_ = 0;
    uint32_t write_fail_count_ = 0;
    uint32_t max_write_time_us_ = 0;
    uint32_t tick_count_ = 0;
};
static_assert(sizeof(BaseStorageHealth) == 20);

struct StorageHealth {
    BaseStorageHealth health;
    SdTimingStats timing;
    
    CSV_IGNORE
    uint8_t padding[4];

    uint64_t disk_size_remaining;
    uint64_t arena_used_bytes;
    uint64_t arena_total_bytes;
};

struct pressures_frame {
    double pressure;
    double pressure_mean;
};
static_assert(sizeof(pressures_frame) == 16);
struct temperature_frame {
    double temperature;
    double temperature_mean;
};
static_assert(sizeof(temperature_frame) == 16);
struct pressure_temperature_frame {
    double pressure;
    double pressure_mean;
    
    double temperature;
    double temperature_mean;
};
static_assert(sizeof(pressure_temperature_frame) == 32);

struct engine_fsm_transition {
    prc::EngineState old_state;
    prc::EngineState new_state;
};
static_assert(sizeof(engine_fsm_transition) == 2);

struct dpr_fsm_transition {
    prc::State old_state;
    prc::State new_state;
};
static_assert(sizeof(dpr_fsm_transition) == 2);

struct valve_transition {
    bool old_open;
    bool new_open;
};
static_assert(sizeof(valve_transition) == 2);

struct ball_valve_transition {
    float old_percent_open;
    float new_percent_open;
};
static_assert(sizeof(ball_valve_transition) == 8);

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

        LOG_MAIN_LOX_TRANSITION,
        LOG_MAIN_ETH_TRANSITION,

        LOG_CHAMBER_FRAME, // { P, T, P_mean, C_mean }
        LOG_CHAMBER_ERROR,

        LOG_EIN_P_FRAME, // { P, P_mean }
        LOG_EIN_T_FRAME, // { T, T_mean }
        LOG_OIN_P_FRAME, // { P, P_mean }
        LOG_OIN_T_FRAME, // { T, T_mean }
        
        LOG_EIN_P_ERROR,
        LOG_OIN_P_ERROR,

        LOG_ERROR // Send an EngineErrorKind
    };
};

namespace lox {

    using OtaPressureFrame = multi::internal::PipelineReturnValue<
        multi::UseOutlier<int, int>,
        multi::UseUnpack,
        3
    >;
    static_assert(sizeof(OtaPressureFrame) == 80);

    struct OtaTemperatureFrame {
        double temperature;
        double temperature_mean;

        uint8_t sensor_id;

        CSV_IGNORE
        uint8_t padding[7];
    };
    static_assert(sizeof(OtaTemperatureFrame) == 24);

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
        LOG_VENT_TRANSITION,
        LOG_BALL_TRANSITION,
        LOG_SAFETY_TRANSITION,

        LOG_FLS, // fill level

        LOG_HPO_FRAME, // { P, P_mean }
        LOG_HPO_ERROR,

        LOG_OTA_P_FRAME, // OtaPressureFrame
        LOG_OTA_T_FRAME, // OtaTemperatureFrame

        LOG_OTA1_P_ERROR,
        LOG_OTA2_P_ERROR,
        LOG_OTA3_P_ERROR,

        LOG_ERROR // Send a LoxErrorKind
    };

}; // namespace lox;



namespace eth {

    using EtaPressureFrame = multi::internal::PipelineReturnValue<
        multi::UseOutlier<int, int>,
        multi::UseUnpack,
        3
    >;
    static_assert(sizeof(EtaPressureFrame) == 80);

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
        
        LOG_VENT_TRANSITION,
        LOG_BALL_TRANSITION,
        LOG_SAFETY_TRANSITION,

        LOG_HPE_FRAME, // { P, P_mean }
        LOG_HPE_ERROR,

        LOG_ETA_P_FRAME, // OtaPressureFrame
        LOG_ETA1_P_ERROR,
        LOG_ETA2_P_ERROR,
        LOG_ETA3_P_ERROR,
        
        LOG_ERROR // Send a LoxErrorKind
    };
};
