
#include "./data.hpp"
#include "./fields.hpp"

using namespace prc;

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define X_raw_FIELD(type, field, cls) \
    type cls::CONCAT(get_, field) () const { \
        return data_.field; \
    } __attribute__((weak)) void cls::CONCAT(set_, field) (type value) { \
        data_.field = value; \
    }
#define X_proc_FIELD(type, field, cls) X_raw_FIELD(type, CONCAT(field, _mean), cls)
#define X_FIELD(type, field, cls) X_raw_FIELD(type, field, cls) X_proc_FIELD(type, field, cls)

X_PRC_SENSORS_STORE_ENGINE(PropSensorsStoreEngine)
X_PRC_SENSORS_STORE_LOX(PropSensorsStoreLox)
X_PRC_SENSORS_STORE_ETH(PropSensorsStoreEth)

#undef CONCAT_IMPL
#undef CONCAT
#undef X_raw_FIELD
#undef X_proc_FIELD
#undef X_FIELD
