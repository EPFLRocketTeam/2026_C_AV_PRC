
#include "Modules/Sensors/pipelines/nothing.hpp"
#include "Modules/Sensors/result.hpp"

template<typename Success, typename Error = NoPipeline>
struct IfPipeline {
private:
    Success success;
    Error   error;
public:
    void ingest (const result<auto, auto> &data) {
        if (data.is_success()) {
            success.ingest(data.get_value());
        } else {
            error.ingest(data.get_error());
        }
    }
};
