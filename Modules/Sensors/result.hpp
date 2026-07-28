#pragma once

#include <variant>
#include <utility>
#include <type_traits>

struct no_error;

template<typename SuccessType, typename ErrorType>
struct result {
private:
    std::variant<SuccessType, ErrorType> var;

    // Private tag-based constructors; use result::success(...)/result::error(...) below.
    template<typename... Args>
    explicit result(std::in_place_index_t<0>, Args&&... args)
        : var(std::in_place_index<0>, std::forward<Args>(args)...) {}

    template<typename... Args>
    explicit result(std::in_place_index_t<1>, Args&&... args)
        : var(std::in_place_index<1>, std::forward<Args>(args)...) {}

public:
    // --- Factory functions -------------------------------------------------

    static result success(SuccessType value) {
        return result(std::in_place_index<0>, std::move(value));
    }

    static result error(ErrorType value) {
        return result(std::in_place_index<1>, std::move(value));
    }

    template<typename... Args>
    static result success_in_place(Args&&... args) {
        return result(std::in_place_index<0>, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static result error_in_place(Args&&... args) {
        return result(std::in_place_index<1>, std::forward<Args>(args)...);
    }

    // --- State queries -------------------------------------------------

    bool is_success() const { return var.index() == 0; }
    bool is_error()   const { return var.index() == 1; }

    explicit operator bool() const { return is_success(); }

    // --- Access -------------------------------------------------

    SuccessType&       get_value()       { return *std::get_if<0>(&var); }
    const SuccessType& get_value() const { return *std::get_if<0>(&var); }

    ErrorType&       get_error()       { return *std::get_if<1>(&var); }
    const ErrorType& get_error() const { return *std::get_if<1>(&var); }

    // Returns the success value, or `fallback` if this result holds an error.
    SuccessType value_or(SuccessType fallback) const {
        return is_success() ? get_value() : fallback;
    }

};