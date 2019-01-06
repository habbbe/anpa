#ifndef PARSER_COMBINATORS_INTERNAL_H
#define PARSER_COMBINATORS_INTERNAL_H

#include <type_traits>
#include <utility>

namespace parse::internal {

/**
 * General helper for evaluating a parser multiple times with an optional separator.
 */
template <typename State, typename Fun, typename Parser, typename Sep = std::nullptr_t>
inline constexpr auto many(State &s, Fun f, Parser p, Sep sep = nullptr) {
    std::size_t successful = 0;
    for (auto res = apply(p, s); res; res = apply(p, s)) {
        ++successful;
        f(*res);
        if constexpr (!std::is_null_pointer_v<Sep>) {
            if (!apply(sep, s)) break;
        }
    }
    return successful;
}

/**
 * Recursive helper for `get_parsed`
 */
template <typename State, typename Iterator, typename Parser, typename... Parsers>
static inline constexpr auto get_parsed_recursive(State &s, Iterator original_position, Parser p, Parsers... ps) {
    if (auto result = apply(p, s)) {
        if constexpr (sizeof...(Parsers) == 0) {
            return s.return_success(s.convert(original_position, s.position));
        } else {
            return get_parsed_recursive(s, original_position, ps...);
        }
    } else {
        return s.template return_fail_change_result<typename std::decay_t<decltype(s)>::default_result_type>(result);
    }
}

// Compile time recursive resolver for lifting of arbitrary number of parsers
template <typename State, typename F, typename Parser, typename... Parsers>
static inline constexpr auto lift_or_rec(State &s, F f, Parser p, Parsers... ps) {
    if (auto result = apply(p, s)) {
        return s.return_success(f(std::move(*result)));
    } else if constexpr (sizeof...(ps) > 0) {
        return lift_or_rec(s, f, ps...);
    } else {
        // All parsers failed
        using result_type = std::decay_t<decltype(f(*result))>;
        if constexpr (State::error_handling) {
            return s.template return_fail<result_type>(result.error());
        } else {
            return s.template return_fail<result_type>();
        }
    }
}

}

#endif // PARSER_COMBINATORS_INTERNAL_H
