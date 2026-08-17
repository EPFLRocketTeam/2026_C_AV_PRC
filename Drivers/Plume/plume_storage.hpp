
#include "Drivers/Plume/plume_driver.hpp"
#include "plume/status.h"

#include "ThirdParty/DataLogger/types.hpp"
#include "stm32h7xx.h"

struct PlumeStorage {
private:
    SDCardInterface *sd_;
public:
    PlumeStorage (SDCardInterface *sd) : sd_(sd) {}

    bool ready () {
        return true;
    }
    void tick () {
        // TODO check if there is an error
        sd_->tick();
    }

    auto withInternalHealth (BaseStorageHealth health) {
        StorageHealth fullHealth;
        fullHealth.health = health;
        fullHealth.timing = sd_timing_snapshot();
        fullHealth.disk_size_remaining = sd_->disk_size_remaining();
        fullHealth.arena_total_bytes = sd_->arena_total_bytes();
        fullHealth.arena_used_bytes = sd_->arena_used_bytes();

        return fullHealth;
    }
    uint32_t now_us () {
        return HAL_GetTick() * 1000;
    }

    void beginTransaction () {
        sd_->beginTransaction();
    }
    void endTransaction () {
        sd_->endTransaction();
    }

    bool write (const uint8_t* payload, uint16_t payload_len) {
        return plume_is_ok(sd_->write(payload, payload_len));
    }
};
