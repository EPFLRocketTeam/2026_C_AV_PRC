
#include <bits/stdc++.h>

using namespace std;

uint32_t g_now_us_ = 0;
uint32_t g_next_us_ = 0;
uint32_t application_now_us () {
    return g_now_us_;
}
void update_us_on_write () {
    if (g_next_us_ > g_now_us_) {
        g_now_us_ = g_next_us_;
    }
}

struct SuccessQueue {
private:
    std::queue<bool> works;

    bool default_ = true;
public:
    SuccessQueue () = default;
    SuccessQueue (bool def) : default_ (def) {}

    void push (bool value) {
        works.push(value);
    }
    bool poll () {
        if (works.empty()) {
            return default_;
        }

        bool result = works.front();
        works.pop();
        return result;
    }
};

struct TransactionContent {
    bool success;
    std::string buffer_copy;
};

struct SampleTestStorage {
private:
    std::stringstream stream_buffer;
    std::stringstream stream_result;

    bool ready_ = true;

    SuccessQueue write_works_;
    std::vector<TransactionContent> transactions_;
public:
    SampleTestStorage (const SampleTestStorage &storage) {
        stream_buffer.str(storage.stream_buffer.str());
        stream_result.str(storage.stream_result.str());

        ready_ = storage.ready_;
        write_works_ = storage.write_works_;
        transactions_ = storage.transactions_;
    }
    void operator=(const SampleTestStorage &storage) {
        stream_buffer.str(storage.stream_buffer.str());
        stream_result.str(storage.stream_result.str());

        ready_ = storage.ready_;
        write_works_ = storage.write_works_;
        transactions_ = storage.transactions_;
    }

    SampleTestStorage () = default;
    SampleTestStorage (bool ready) : ready_(ready) {}
    SampleTestStorage (bool ready, SuccessQueue write_works) : ready_(ready), write_works_(write_works) {}

    std::string str () { return stream_result.str(); }

    bool ready () { return ready_; }
    void tick () {
        stream_result << stream_buffer.str();
        stream_buffer.str("");
    }
    void clear () {
        stream_buffer.str("");
        stream_result.str("");
        transactions_.clear();
    }

    StorageHealth withInternalHealth (StorageHealth health) {
        return health;
    }

    uint32_t now_us () {
        return application_now_us();
    }

    void beginTransaction () {
        transactions_.push_back({ true, stream_buffer.str() });
    }
    void endTransaction () {
        if (transactions_.size() == 0) return ;

        TransactionContent tr = transactions_.back();
        transactions_.pop_back();
        
        if (!tr.success) {
            stream_buffer.str(tr.buffer_copy);
        }
    }

    bool write (const uint8_t* payload, uint16_t payload_len) {
        update_us_on_write();

        if (write_works_.poll()) {
            for (uint16_t offset = 0; offset < payload_len; offset ++) {
                stream_buffer << payload[offset];
            }

            return true;
        }
        
        if (transactions_.size()) {
            transactions_.back().success = false;
        }

        return false;
    }
};
