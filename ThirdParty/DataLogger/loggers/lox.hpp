#pragma once
#include "ThirdParty/DataLogger/base.hpp"

template<typename Storage>
using LoxBase = BaseDataLogger<LOX_LOGGER_MAGIC, lox::RecordType, Storage>;

template<typename Storage>
struct LoxDataLogger : public LoxBase<Storage> {
public:
    LoxDataLogger () = default;
    LoxDataLogger (Storage& storage) : LoxBase<Storage>(storage) {}

    void logDataDump (prc::DataDump dump) {
        this->writeRecord(lox::RecordType::LOG_DATA_DUMP, &dump, sizeof(dump));
    }
    
    void logFLS (double frame) {
        this->writeRecord(lox::RecordType::LOG_FLS, &frame, sizeof(frame));
    }
    
    void logHPO (pressures_frame frame) {
        this->writeRecord(lox::RecordType::LOG_HPO_FRAME, &frame, sizeof(frame));
    }

    void logOTAPressureFrame (const lox::OtaPressureFrame &frame) {
        this->writeRecord(lox::RecordType::LOG_OTA_P_FRAME, &frame, sizeof(frame));
    }
    void logOTATemperatureFrame (const lox::OtaTemperatureFrame &frame) {
        this->writeRecord(lox::RecordType::LOG_OTA_T_FRAME, &frame, sizeof(frame));
    }
    
    void logFsmTransition (dpr_fsm_transition frame) {
        this->writeRecord(lox::RecordType::LOG_FSM_TRANSITION, &frame, sizeof(frame));
    }

    void logError (lox::ErrorKind kind) {
        this->writeRecord(lox::RecordType::LOG_ERROR, &kind, sizeof(kind));
    }
};
