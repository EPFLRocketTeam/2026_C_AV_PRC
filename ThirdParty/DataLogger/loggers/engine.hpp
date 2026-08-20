#pragma once
#include "ThirdParty/DataLogger/base.hpp"

template<typename Storage>
using EngineBase = BaseDataLogger<ENGINE_LOGGER_MAGIC, engine::RecordType, Storage>;

template<typename Storage>
struct EngingDataLogger : public EngineBase<Storage> {
public:
    EngingDataLogger () = default;
    EngingDataLogger (Storage& storage) : EngineBase<Storage>(storage) {}

    void logDataDump (prc::DataDump dump) {
        this->writeRecord(engine::RecordType::LOG_DATA_DUMP, &dump, sizeof(dump));
    }
    
    void logChamberFrame (pressure_temperature_frame frame) {
        this->writeRecord(engine::RecordType::LOG_CHAMBER_FRAME, &frame, sizeof(frame));
    }
    
    void logEinPFrame (pressures_frame frame) {
        this->writeRecord(engine::RecordType::LOG_EIN_P_FRAME, &frame, sizeof(frame));
    }
    void logEinTFrame (temperature_frame frame) {
        this->writeRecord(engine::RecordType::LOG_EIN_T_FRAME, &frame, sizeof(frame));
    }
    
    void logOinPFrame (pressures_frame frame) {
        this->writeRecord(engine::RecordType::LOG_OIN_P_FRAME, &frame, sizeof(frame));
    }
    void logOinTFrame (temperature_frame frame) {
        this->writeRecord(engine::RecordType::LOG_OIN_T_FRAME, &frame, sizeof(frame));
    }

    void logFsmTransition (engine_fsm_transition frame) {
        this->writeRecord(engine::RecordType::LOG_FSM_TRANSITION, &frame, sizeof(frame));
    }

    void logError (engine::ErrorKind kind) {
        this->writeRecord(engine::RecordType::LOG_ERROR, &kind, sizeof(kind));
    }
};
