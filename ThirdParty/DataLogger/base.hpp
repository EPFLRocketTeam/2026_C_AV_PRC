
#include "ThirdParty/DataLogger/types.hpp"

template<uint8_t Magic, typename EnumKinds, typename Storage>
struct BaseDataLogger {
private:
    Storage storage_;

    StorageHealth health_;
protected:
    BaseDataLogger () = default;
    BaseDataLogger (Storage& storage) : storage_(storage) {}

    void writeRecord (EnumKinds recordType, const void* payload, uint16_t payload_len) {
        if (!storage_.ready()) return;

        LogHeader hdr;
        hdr.magic        = Magic;
        hdr.record_type  = static_cast<uint8_t>(recordType);
        hdr.length       = payload_len;
        hdr.timestamp_us = storage_.now_us();

        const uint32_t t0 = hdr.timestamp_us;

        // Write header + payload as a single contiguous write.
        // Plume's ring buffer handles the byte-level copy.
        uint8_t buf[sizeof(LogHeader) + 1024]; // stack buffer for small records
        
        bool result;
        if (sizeof(LogHeader) + payload_len <= sizeof(buf)) {
            memcpy(buf, &hdr, sizeof(hdr));
            memcpy(buf + sizeof(hdr), payload, payload_len);
            result = storage_.write(buf, sizeof(hdr) + payload_len);
        } else {
            // Large record: write header then payload separately
            storage_.beginTransaction();
            storage_.write(reinterpret_cast<const uint8_t*>(&hdr), sizeof(hdr));
            result = storage_.write(reinterpret_cast<const uint8_t*>(payload), payload_len);
            storage_.endTransaction();
        }

        const uint32_t elapsed_us = storage_.now_us() - t0;
        if (elapsed_us > health_.max_write_time_us_) health_.max_write_time_us_ = elapsed_us;

        health_.write_count_++;
        if (!result) {
            health_.write_fail_count_++;
        } else {
            health_.bytes_written_ += sizeof(hdr) + payload_len;
        }
    }
public:
    void tick () {
        storage_.tick();

        health_.tick_count_ ++;
    }

    Storage &getStorage () {
        return storage_;
    }

    void logStorageHealth () {
        auto fullHealth = storage_.withInternalHealth(health_);

        writeRecord(EnumKinds::LOG_HEALTH, &fullHealth, sizeof(fullHealth));
    }
};
