#pragma once
#include "ThirdParty/DataLogger/base.hpp"

template<typename Storage>
using EthBase = BaseDataLogger<ETH_LOGGER_MAGIC, eth::RecordType, Storage>;

template<typename Storage>
struct EthDataLogger : public EthBase<Storage> {
public:
    EthDataLogger () : EthBase<Storage>() {};
    EthDataLogger (Storage& storage) : EthBase<Storage>(storage) {}

    void logDataDump (prc::DataDump dump) {
        this->writeRecord(eth::RecordType::LOG_DATA_DUMP, &dump, sizeof(dump));
    }
    
    void logHPE (pressures_frame frame) {
        this->writeRecord(eth::RecordType::LOG_HPE_FRAME, &frame, sizeof(frame));
    }
    void logHPEPressureError (const sensata::SensataError &error) {
        this->writeRecord(eth::RecordType::LOG_HPE_ERROR, &error, sizeof(error));
    }

    void logETAPressureFrame (const eth::EtaPressureFrame &frame) {
        this->writeRecord(eth::RecordType::LOG_ETA_P_FRAME, &frame, sizeof(frame));
    }

    void logETA1PressureError (const sensata::SensataError &error) {
        this->writeRecord(eth::RecordType::LOG_ETA1_P_ERROR, &error, sizeof(error));
    }
    void logETA2PressureError (const sensata::SensataError &error) {
        this->writeRecord(eth::RecordType::LOG_ETA2_P_ERROR, &error, sizeof(error));
    }
    void logETA3PressureError (const sensata::SensataError &error) {
        this->writeRecord(eth::RecordType::LOG_ETA3_P_ERROR, &error, sizeof(error));
    }
    
    void logFsmTransition (dpr_fsm_transition frame) {
        this->writeRecord(eth::RecordType::LOG_FSM_TRANSITION, &frame, sizeof(frame));
    }
    void logVentEthTransition (valve_transition frame) {
        this->writeRecord(eth::RecordType::LOG_VENT_TRANSITION, &frame, sizeof(frame));
    }
    void logSafetyEthTransition (valve_transition frame) {
        this->writeRecord(eth::RecordType::LOG_SAFETY_TRANSITION, &frame, sizeof(frame));
    }
    void logBallValveEthTransition (ball_valve_transition frame) {
        this->writeRecord(eth::RecordType::LOG_BALL_TRANSITION, &frame, sizeof(frame));
    }

    void logError (eth::ErrorKind kind) {
        this->writeRecord(eth::RecordType::LOG_ERROR, &kind, sizeof(kind));
    }
};
