#ifndef PARSER_RESULT_H
#define PARSER_RESULT_H

#include <type_traits>
#include <variant>
#include <optional>

namespace parse {
template <typename R, typename ErrorType>
struct result {
    constexpr static auto has_error_handling = !std::is_void_v<ErrorType>;
    typename std::conditional<has_error_handling, std::variant<ErrorType, R>, std::optional<R>>::type res;

    template <size_t N, typename... V>
    result(std::in_place_index_t<N> p, V&&...v) : res{p, std::forward<V>(v)...} {}

    template <typename... V>
    result(std::in_place_t t, V&&...v) : res{std::optional<R>(t, std::forward<V>(v)...)} {}

    result() : res{std::nullopt} {}

    auto const& operator*() {
        if constexpr (has_error_handling) {
            return std::get<1>(res);
        } else {
            return *res;
        }
    }

    operator bool() {
        if constexpr (has_error_handling) {
            return res.index() == 1;
        } else {
            return res.operator bool();
        }
    }

    auto const& error() {
        static_assert(has_error_handling, "No error handling");
        return std::get<0>(res);
    }
};

using default_error_type = const char*;
constexpr auto error_handling = false;


}

#endif // PARSER_RESULT_H
