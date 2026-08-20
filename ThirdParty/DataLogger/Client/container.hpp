
#include <stdexcept>
#include <assert.h>
#include "./channel.hpp"
#include "../types.hpp"

#define X_CHANNELS \
    X_CHANNEL(ENGINE_LOGGER_MAGIC, engine, LOG_HEALTH,         StorageHealth,              "prc/engine/StorageHealth.csv") \
    X_CHANNEL(ENGINE_LOGGER_MAGIC, engine, LOG_DATA_DUMP,      prc::DataDump,              "prc/engine/DataDump.csv") \
    X_CHANNEL(ENGINE_LOGGER_MAGIC, engine, LOG_FSM_TRANSITION, engine_fsm_transition,      "prc/engine/FsmTransitions.csv") \
    X_CHANNEL(ENGINE_LOGGER_MAGIC, engine, LOG_CHAMBER_FRAME,  pressure_temperature_frame, "prc/engine/sensors/Chamber.csv") \
    X_CHANNEL(ENGINE_LOGGER_MAGIC, engine, LOG_EIN_P_FRAME,    pressures_frame,            "prc/engine/sensors/EIN-P.csv") \
    X_CHANNEL(ENGINE_LOGGER_MAGIC, engine, LOG_EIN_T_FRAME,    temperature_frame,          "prc/engine/sensors/EIN-T.csv") \
    X_CHANNEL(ENGINE_LOGGER_MAGIC, engine, LOG_OIN_P_FRAME,    pressures_frame,            "prc/engine/sensors/OIN-P.csv") \
    X_CHANNEL(ENGINE_LOGGER_MAGIC, engine, LOG_OIN_T_FRAME,    temperature_frame,          "prc/engine/sensors/OIN-T.csv") \
    X_CHANNEL(ENGINE_LOGGER_MAGIC, engine, LOG_ERROR,          engine::ErrorKind,          "prc/engine/Errors.csv") \
    \
    X_CHANNEL(LOX_LOGGER_MAGIC, lox, LOG_HEALTH,         StorageHealth,              "prc/lox/StorageHealth.csv") \
    X_CHANNEL(LOX_LOGGER_MAGIC, lox, LOG_DATA_DUMP,      prc::DataDump,              "prc/lox/DataDump.csv") \
    X_CHANNEL(LOX_LOGGER_MAGIC, lox, LOG_FSM_TRANSITION, dpr_fsm_transition,         "prc/lox/FsmTransitions.csv") \
    X_CHANNEL(LOX_LOGGER_MAGIC, lox, LOG_FLS,            double,                     "prc/lox/sensors/FLS.csv") \
    X_CHANNEL(LOX_LOGGER_MAGIC, lox, LOG_HPO_FRAME,      pressures_frame,            "prc/lox/sensors/HPO.csv") \
    X_CHANNEL(LOX_LOGGER_MAGIC, lox, LOG_OTA_P_FRAME,    lox::OtaPressureFrame,      "prc/lox/sensors/OTA-P.csv") \
    X_CHANNEL(LOX_LOGGER_MAGIC, lox, LOG_OTA_T_FRAME,    lox::OtaTemperatureFrame,   "prc/lox/sensors/OTA-T.csv") \
    X_CHANNEL(LOX_LOGGER_MAGIC, lox, LOG_ERROR,          lox::ErrorKind,             "prc/lox/Errors.csv") \
    \
    X_CHANNEL(ETH_LOGGER_MAGIC, eth, LOG_HEALTH,         StorageHealth,              "prc/eth/StorageHealth.csv") \
    X_CHANNEL(ETH_LOGGER_MAGIC, eth, LOG_DATA_DUMP,      prc::DataDump,              "prc/eth/DataDump.csv") \
    X_CHANNEL(ETH_LOGGER_MAGIC, eth, LOG_FSM_TRANSITION, dpr_fsm_transition,         "prc/eth/FsmTransitions.csv") \
    X_CHANNEL(ETH_LOGGER_MAGIC, eth, LOG_HPE_FRAME,      pressures_frame,            "prc/eth/sensors/HPE.csv") \
    X_CHANNEL(ETH_LOGGER_MAGIC, eth, LOG_ETA_P_FRAME,    eth::EtaPressureFrame,      "prc/eth/sensors/ETA.csv") \
    X_CHANNEL(ETH_LOGGER_MAGIC, eth, LOG_ERROR,          eth::ErrorKind,             "prc/eth/Errors.csv")

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

#define CHANNEL_VAR(ns, RecordType) CONCAT(Channel_, CONCAT(ns, _##RecordType))

struct CsvChannelContainer {
private:
    #define X_CHANNEL(Magic, ns, RecordType, Typename, FileName) \
        CsvChannel<Typename> CHANNEL_VAR(ns, RecordType);
    X_CHANNELS
    #undef X_CHANNEL

    bool has_init = false;
    uint8_t exp_magic = 0;
    bool init_with_magic (uint8_t magic) {
        if (has_init) return magic == exp_magic;
        exp_magic = magic;
        has_init = true;

        #define X_CHANNEL(Magic, ns, RecordType, Typename, FileName) \
            if (Magic == magic) { CHANNEL_VAR(ns, RecordType).write_header(); }
        X_CHANNELS
        #undef X_CHANNEL
        return true;
    }
public:
    CsvChannelContainer (
        std::function<std::ostream&(const std::string&)> get_channel_stream
    ) {
        #define X_CHANNEL(Magic, ns, RecordType, Typename, FileName) \
            CHANNEL_VAR(ns, RecordType) = CsvChannel<Typename>(get_channel_stream, FileName);
        X_CHANNELS
        #undef X_CHANNEL
    }

    void ingest (LogHeader header, const void* payload) {
        if (!init_with_magic(header.magic)) {
            throw std::runtime_error("Invalid magic.");
        }

        bool found = false;
        
        #define X_CHANNEL(Magic, ns, RecordType, Typename, FileName) \
            if (Magic == header.magic && ns::RecordType == header.record_type) { \
                if (((int) sizeof(Typename)) != ((int) header.length)) { \
                    printf(#Typename " : local = %d, remote = %d\n", (int) sizeof(Typename), (int) header.length); \
                    assert(false); \
                } \
                CHANNEL_VAR(ns, RecordType).aggregate(header.timestamp_us, *((const Typename*) payload)); \
                found = true; \
            }
        X_CHANNELS
        #undef X_CHANNEL

        if (!found) {
            throw std::runtime_error("Invalid record type.");
        }
    }

};

#undef CONCAT_IMPL
#undef CONCAT