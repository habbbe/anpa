#ifndef PARSIMON_RESULT_H
#define PARSIMON_RESULT_H

#include <type_traits>
#include <variant>
#include <optional>
#include "parsimon/parse_error.h"

namespace parsimon {

/**
 * A parser result. It can either be present or not present.
 */
template <typename R, typename ErrorType>
struct result {
    constexpr static auto has_error_handling = !std::is_void_v<ErrorType>;
    typename std::conditional_t<has_error_handling, std::variant<ErrorType, R>, std::optional<R>> res;

    template <size_t N, typename... V,
              typename Type = std::conditional_t<N==0, ErrorType, R>,
              typename std::enable_if_t<std::is_constructible_v<Type, V...>, int> = 0>
    constexpr result(std::in_place_index_t<N> p, V&&... v) : res{p, std::forward<V>(v)...} {}

    template <size_t N, typename... V,
              typename Type = std::conditional_t<N==0, ErrorType, R>,
              typename std::enable_if_t<!std::is_constructible_v<Type, V...>, int> = 0>
    constexpr result(std::in_place_index_t<N> p, V&&... v) : res{p, Type{std::forward<V>(v)...}} {}

    template <typename... V, typename std::enable_if_t<std::is_constructible_v<R, V...>, int> = 0>
    constexpr result(std::in_place_t t, V&&... v) : res{t, std::forward<V>(v)...} {}

    template <typename... V, typename std::enable_if_t<!std::is_constructible_v<R, V...>, int> = 0>
    constexpr result(std::in_place_t t, V&&... v) : res{t, R{std::forward<V>(v)...}} {}

    constexpr result() : res{std::nullopt} {}

    ///@{
    /**
     * Get the contained value
     *
     * If a result is not present, calling this method is undefined behavior,
     * so check result::has_value() first.
     */
    constexpr decltype(auto) get_value() & {
        if constexpr (has_error_handling) {
            return std::get<1>(res);
        } else {
            return *res;
        }
    }

    constexpr decltype(auto) get_value() && { return std::move(get_value()); }
    constexpr decltype(auto) get_value() const& { return const_cast<result*>(this)->get_value(); }
    ///@}

    ///@{
    /** @see result::get_value() */
    constexpr decltype(auto) operator*() & { return get_value(); }
    constexpr decltype(auto) operator*() && { return static_cast<result&&>(*this).get_value(); }
    constexpr decltype(auto) operator*() const& { return get_value(); }

    constexpr decltype(auto) operator->() { return &get_value(); }
    constexpr decltype(auto) operator->() const { return &get_value(); }
    ///@}

    ///@{
    /** Check if the result is present */
    constexpr bool has_value() const {
        if constexpr (has_error_handling) {
            return res.index() == 1;
        } else {
            return res.operator bool();
        }
    }

    constexpr operator bool() const { return has_value(); }
    ///@}

    ///@{
    /** Get the parse error
     *
     * If a result is present, calling this method is undefined behavior,
     * so check !result::has_value() first.
     */
    constexpr decltype(auto) error() {
        static_assert(has_error_handling, "No error handling");
        return std::get<0>(res);
    }

    constexpr decltype(auto) error() const { return const_cast<result*>(this)->error(); }
    ///@}
};

}

#endif // PARSIMON_RESULT_H
