#pragma once
#include "ThirdParty/DataLogger/base.hpp"

template<typename Storage>
struct EngingDataLogger : public BaseDataLogger<ENGINE_LOGGER_MAGIC, engine::RecordType, Storage> {
public:
    EngingDataLogger () = default;
    EngingDataLogger (Storage& storage) : BaseDataLogger(storage) {}

    void logDataDump (prc::DataDump &dump) {
        writeRecord(engine::RecordType::LOG_DATA_DUMP, &dump, sizeof(dump));
    }
    
    void logChamberFrame (pressure_temperature_frame frame) {
        writeRecord(engine::RecordType::LOG_CHAMBER_FRAME, &frame, sizeof(frame));
    }
    
    void logEinPFrame (pressures_frame frame) {
        writeRecord(engine::RecordType::LOG_EIN_P_FRAME, &frame, sizeof(frame));
    }
    void logEinTFrame (temperature_frame frame) {
        writeRecord(engine::RecordType::LOG_EIN_T_FRAME, &frame, sizeof(frame));
    }
    
    void logOinPFrame (pressures_frame frame) {
        writeRecord(engine::RecordType::LOG_OIN_P_FRAME, &frame, sizeof(frame));
    }
    void logOinTFrame (temperature_frame frame) {
        writeRecord(engine::RecordType::LOG_OIN_T_FRAME, &frame, sizeof(frame));
    }

    // TODO choose the type of bundled old + new, find types of
    //   FSM states.
    //void logFsmTransition (??? old_fsm, ??? new_fsm) {
    //    writeRecord(engine::RecordType::LOG_FSM_TRANSITION, ???, ???)
    //}

    void logError (engine::ErrorKind kind) {
        writeRecord(engine::RecordType::LOG_ERROR, &kind, sizeof(EngineErrorKind));
    }
};
