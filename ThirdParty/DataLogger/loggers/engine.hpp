#pragma once
#include "ThirdParty/DataLogger/base.hpp"

template<typename Storage>
using EngineBase = BaseDataLogger<ENGINE_LOGGER_MAGIC, engine::RecordType, Storage>;

template<typename Storage>
struct EngineDataLogger : public EngineBase<Storage> {
public:
    EngineDataLogger () : EngineBase<Storage>() {};
    EngineDataLogger (Storage& storage) : EngineBase<Storage>(storage) {}

    void logDataDump (prc::DataDump dump) {
        this->writeRecord(engine::RecordType::LOG_DATA_DUMP, &dump, sizeof(dump));
    }
    
    void logChamberFrame (pressure_temperature_frame frame) {
        this->writeRecord(engine::RecordType::LOG_CHAMBER_FRAME, &frame, sizeof(frame));
    }
    void logChamberError (const sensata::SensataError &error) {
        this->writeRecord(engine::RecordType::LOG_CHAMBER_ERROR, &error, sizeof(error));
    }
    
    void logEinPFrame (pressures_frame frame) {
        this->writeRecord(engine::RecordType::LOG_EIN_P_FRAME, &frame, sizeof(frame));
    }
    void logEinPError (const sensata::SensataError &error) {
        this->writeRecord(engine::RecordType::LOG_EIN_P_ERROR, &error, sizeof(error));
    }

    void logEinTFrame (temperature_frame frame) {
        this->writeRecord(engine::RecordType::LOG_EIN_T_FRAME, &frame, sizeof(frame));
    }
    
    void logOinPFrame (pressures_frame frame) {
        this->writeRecord(engine::RecordType::LOG_OIN_P_FRAME, &frame, sizeof(frame));
    }
    void logOinPError (const sensata::SensataError &error) {
        this->writeRecord(engine::RecordType::LOG_OIN_P_ERROR, &error, sizeof(error));
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
