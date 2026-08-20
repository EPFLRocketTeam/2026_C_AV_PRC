#pragma once
#include "ThirdParty/DataLogger/base.hpp"

template<typename Storage>
using EthBase = BaseDataLogger<ETH_LOGGER_MAGIC, eth::RecordType, Storage>;

template<typename Storage>
struct EthDataLogger : public EthBase<Storage> {
public:
    EthDataLogger () = default;
    EthDataLogger (Storage& storage) : EthBase<Storage>(storage) {}

    void logDataDump (prc::DataDump dump) {
        this->writeRecord(eth::RecordType::LOG_DATA_DUMP, &dump, sizeof(dump));
    }
    
    void logHPE (pressures_frame frame) {
        this->writeRecord(eth::RecordType::LOG_HPE_FRAME, &frame, sizeof(frame));
    }

    void logETAPressureFrame (const eth::EtaPressureFrame &frame) {
        this->writeRecord(eth::RecordType::LOG_ETA_P_FRAME, &frame, sizeof(frame));
    }
    
    void logFsmTransition (dpr_fsm_transition frame) {
        this->writeRecord(eth::RecordType::LOG_FSM_TRANSITION, &frame, sizeof(frame));
    }

    void logError (eth::ErrorKind kind) {
        this->writeRecord(eth::RecordType::LOG_ERROR, &kind, sizeof(kind));
    }
};
