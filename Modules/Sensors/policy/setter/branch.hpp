
#pragma once
#include <utility>
#include <functional>

template<typename... Setters>
struct BranchSetterPolicy {
private:
    std::tuple<Setters...> setters_;

public:
    void set(const auto &data) {
        std::apply([&data](auto&... setter) {
            (setter.set(data), ...);
        }, setters_);
    }
};
