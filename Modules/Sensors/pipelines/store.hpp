
#pragma once

template<
    typename Storage,
    auto Setter
>
struct StoreSetterPolicy {
private:
    Storage storage;
public:
    inline void ingest (const auto &data) {
        auto &ref = storage.ref();

        (ref.*Setter)(data);
    }
};
