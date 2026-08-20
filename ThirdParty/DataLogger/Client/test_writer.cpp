
#include "../base.hpp"
#include "../loggers/engine.hpp"
#include "../loggers/eth.hpp"
#include "../loggers/lox.hpp"

#include <fstream>

struct UnixStorage {
private:
    std::ofstream stream;
public:
    UnixStorage () = default;
    UnixStorage (const std::string &target) : stream (target) {} 

    bool ready () {
        return true;
    }
    void tick () {}

    auto withInternalHealth (BaseStorageHealth health) {
        StorageHealth fullHealth;
        fullHealth.health = health;
        fullHealth.timing = {};
        fullHealth.disk_size_remaining = 0;
        fullHealth.arena_total_bytes = 0;
        fullHealth.arena_used_bytes = 0;

        return fullHealth;
    }
    uint32_t now_us () {
        return 0;
    }

    void beginTransaction () {}
    void endTransaction () {}

    bool write (const uint8_t* payload, uint16_t payload_len) {
        stream.write((const char*) payload, payload_len);
        return true;
    }
};

int main (void) {
    UnixStorage storage("log.bin");
    EthDataLogger<UnixStorage> logger(storage);
    logger.logFsmTransition({
        .old_state = prc::State::MANUAL,
        .new_state = prc::State::INITIALIZE_PASSIVATE
    });
}
