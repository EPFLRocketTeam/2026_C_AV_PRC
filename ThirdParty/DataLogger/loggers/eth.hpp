#pragma once
#include "ThirdParty/DataLogger/base.hpp"

template<typename Storage>
struct EthDataLogger : public BaseDataLogger<ETH_LOGGER_MAGIC, EngineRecordType, Storage> {
public:
    EthDataLogger () = default;
    EthDataLogger (Storage& storage) : BaseDataLogger(storage) {}

    void logDataDump (prc::DataDump &dump) {
        writeRecord(eth::RecordType::LOG_DATA_DUMP, &dump, sizeof(dump));
    }
    
    void logHPE (pressures_frame frame) {
        writeRecord(eth::RecordType::LOG_HPE_FRAME, &frame, sizeof(frame));
    }

    void logETAPressureFrame (const EtaPressureFrame &frame) {
        writeRecord(eth::RecordType::LOG_ETA_P_FRAME, &frame, sizeof(frame));
    }
    
    void logFsmTransition (dpr_fsm_transition frame) {
        writeRecord(eth::RecordType::LOG_FSM_TRANSITION, &frame, sizeof(frame));
    }

    void logError (eth::ErrorKind kind) {
        writeRecord(eth::RecordType::LOG_ERROR, &kind, sizeof(EngineErrorKind));
    }
};
