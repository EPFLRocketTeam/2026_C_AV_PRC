#pragma once
#include "./fields.hpp"
#include "../istore.hpp"

namespace prc {

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define X_raw_FIELD(type, field) type field;
#define X_proc_FIELD(type, field) type CONCAT(field, _mean);
#define X_FIELD(type, field) X_raw_FIELD(type, field) X_proc_FIELD(type, field)

struct PropSensorsEngine { X_PRC_SENSORS_STORE_ENGINE(); };
struct PropSensorsLox { X_PRC_SENSORS_STORE_LOX(); };
struct PropSensorsEth { X_PRC_SENSORS_STORE_ETH(); };

static_assert(sizeof(PropSensorsEngine) == 96);
static_assert(sizeof(PropSensorsLox) == 128);
static_assert(sizeof(PropSensorsEth) == 48);

#undef CONCAT_IMPL
#undef CONCAT
#undef X_raw_FIELD
#undef X_proc_FIELD
#undef X_FIELD

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define X_raw_FIELD(type, field) type CONCAT(get_, field) () const; void CONCAT(set_, field) (type value);
#define X_proc_FIELD(type, field) X_raw_FIELD(type, CONCAT(field, _mean))
#define X_FIELD(type, field) X_raw_FIELD(type, field) X_proc_FIELD(type, field)

class PropSensorsStoreEngine : public IStore<PropSensorsEngine> {
public:
  X_PRC_SENSORS_STORE_ENGINE();
};
class PropSensorsStoreLox : public IStore<PropSensorsLox> {
public:
  X_PRC_SENSORS_STORE_LOX();
};
class PropSensorsStoreEth : public IStore<PropSensorsEth> {
public:
  X_PRC_SENSORS_STORE_ETH();
};

#undef CONCAT_IMPL
#undef CONCAT
#undef X_raw_FIELD
#undef X_proc_FIELD
#undef X_FIELD

};
