
#include "Modules/Sensors/pipelines/nothing.hpp"
#include "Modules/Sensors/result.hpp"

template<typename Success, typename Error = NoPipeline>
struct IfPipeline {
private:
    Success success;
    Error   error;
public:
    template<typename U, typename V>
    void ingest (const result<U, V> &data) noexcept {
        if (data.is_success()) {
            success.ingest(data.get_value());
        } else {
            error.ingest(data.get_error());
        }
    }
};
