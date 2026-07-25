
#pragma once
#include "Application/Data/data.hpp"

template<
    typename Storage,
    auto Setter
>
struct StoreSetterPolicy {
private:
    Storage storage;
public:
    void set (const auto &data) {
        auto &ref = storage.ref();

        (ref.*Setter)(data);
    }
};

template<auto Field>
struct PrcStorage {
    auto& ref () const {
        return (prc::PrcStore::get_instance().*Field);
    }
};

#define ENGINE_SETTER_POLICY(member) StoreSetterPolicy<PrcStorage<\
    &prc::PrcStore::propSensorsStoreEngine>, &member>;
#define ETH_SETTER_POLICY(member) StoreSetterPolicy<PrcStorage<\
    &prc::PrcStore::propSensorsStoreEth>, &member>;
#define LOX_SETTER_POLICY(member) StoreSetterPolicy<PrcStorage<\
    &prc::PrcStore::propSensorsStoreLox>, &member>;
