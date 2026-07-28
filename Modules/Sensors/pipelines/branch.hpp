
#pragma once
#include <cstdint>
#include <utility>
#include <functional>

template<typename... Setters>
struct BranchPipeline {
private:
    std::tuple<Setters...> setters_;

public:
    inline void ingest(const auto &data) {
        std::apply([&data](auto&... setter) {
            (setter.ingest(data), ...);
        }, setters_);
    }
};

template<typename... Setters>
struct BranchUnpackPipeline {
private:
    std::tuple<Setters...> setters_;
    
    static constexpr std::size_t NumberSetters = sizeof...(Setters);

public:
    inline auto ingest (const std::array<auto, NumberSetters> &data) {
        std::apply([&data](auto&... setter) {
            size_t idx = 0;
            (setter.ingest(data[idx ++]), ...);
        }, setters_);

        return data;
    }
};
