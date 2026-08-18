
#include "./types.hpp"
#include <ostream>
#include <meta>
#include <type_traits>

namespace csv {

template<typename T>
struct value {
    using type = T;

    T value;
    bool first = true;
};

};

template <typename T>
std::ostream& operator<<(std::ostream& os, const csv::value<T>& x);

#define CSV_VALUE_BASE_FN(type) \
    std::ostream& operator<< (std::ostream& os, const csv::value<type> &x) { \
        if (!x.first) os << ","; \
        os << x.value; \
        return os; \
    }

#define X(type) \
    template<> \
    CSV_VALUE_BASE_FN(type)
X_PRIMITIVE_TYPES
#undef X

template<typename T>
    requires std::is_enum_v<T>
std::ostream& operator<<(std::ostream &os, const csv::value<T>& x) {
    if (!x.first) os << ",";

    static constexpr auto enumerators = std::define_static_array(
        std::meta::enumerators_of(^^T)
    );

    bool found = false;
    template for (constexpr auto e : enumerators) {
        if (!found && x.value == [:e:]) {
            os << std::meta::identifier_of(e);
            found = true;
        }
    }

    if (!found) {
        os << "UNKNOWN";
    }

    return os;
}

template<typename T, size_t N>
std::ostream& operator<<(std::ostream &os, const csv::value<std::array<T, N>> &x) {
    for (size_t i = 0; i < N; i ++) {
        os << csv::value<T>{ x.value[i], x.first && (i == 0) };
    }
    return os;
}

template<typename T>
    requires std::is_aggregate_v<T>
std::ostream& operator<<(std::ostream &os, const csv::value<T>& x) {
    bool first = x.first;

    static constexpr auto members =
        std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())
        );

    template for (constexpr auto member : members) {
        constexpr bool skip = ([member] {
            for (auto anno : std::meta::annotations_of(member))
                if (std::meta::remove_cv(std::meta::type_of(anno)) == ^^csv::ignore_t)
                    return true;
            return false;
        })();

        if constexpr (skip) continue;
        
        using FieldT = [: std::meta::type_of(member) :];

        os << csv::value<FieldT>{ x.value.[: member :], first };
        first = false;
    }

    return os;
}
