#pragma once
#include "ThirdParty/DataLogger/base.hpp"

template<typename Storage>
struct LoxDataLogger : public BaseDataLogger<ENGINE_LOGGER_MAGIC, EngineRecordType, Storage> {
public:
    LoxDataLogger () = default;
    LoxDataLogger (Storage& storage) : BaseDataLogger(storage) {}

    void logDataDump (prc::DataDump &dump) {
        writeRecord(lox::RecordType::LOG_DATA_DUMP, &dump, sizeof(dump));
    }
    
    void logFLS (double frame) {
        writeRecord(lox::RecordType::LOG_FLS, &frame, sizeof(frame));
    }
    
    void logHPO (pressures_frame frame) {
        writeRecord(lox::RecordType::LOG_HPO_FRAME, &frame, sizeof(frame));
    }

    void logOTAPressureFrame (const OtaPressureFrame &frame) {
        writeRecord(lox::RecordType::LOG_OTA_P_FRAME, &frame, sizeof(frame));
    }
    void logOTATemperatureFrame (const OtaTemperatureFrame &frame) {
        writeRecord(lox::RecordType::LOG_OTA_T_FRAME, &frame, sizeof(frame));
    }
    
    // TODO choose the type of bundled old + new, find types of
    //   FSM states.
    //void logFsmTransition (??? old_fsm, ??? new_fsm) {
    //    writeRecord(EngineRecordType::LOG_FSM_TRANSITION, ???, ???)
    //}

    void logError (lox::ErrorKind kind) {
        writeRecord(lox::RecordType::LOG_ERROR, &kind, sizeof(EngineErrorKind));
    }
};
