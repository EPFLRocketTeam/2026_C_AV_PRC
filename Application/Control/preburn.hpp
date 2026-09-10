
#include <cstddef>
#include <cstdint>

struct PreburnRegulator {
private:
    bool enabled_ = false;
    bool is_first_ = false;
    uint32_t until_;
public:
    void registerOpen (uint32_t until) {
        until_ = until;
        enabled_ = true;
        is_first_ = true;
    }
    bool isRunning (uint32_t time) {
        return enabled_ && time <= until_;
    }
    bool isFirstTick () {
        if (is_first_) {
            is_first_ = false;
            return true;
        }
        return false;
    }
    bool isEndTick () {
        if (enabled_) {
            enabled_ = false;
            return true;
        }
        return false;
    }
};

extern PreburnRegulator preburnRegulator;
