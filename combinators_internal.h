#ifndef PARSER_COMBINATORS_INTERNAL_H
#define PARSER_COMBINATORS_INTERNAL_H

#include <type_traits>
#include <utility>
#include <valgrind/callgrind.h>
#include "types.h"

namespace parse::internal {

/**
 * General helper for evaluating a parser multiple times with an optional separator.
 */
template <bool FailOnNoSuccess = false,
          typename State,
          typename Parser,
          typename Sep = none,
          typename Fun = none>
inline constexpr auto many(State& s,
                           Parser p,
                           [[maybe_unused]] Sep sep = {},
                           [[maybe_unused]] Fun f = {}) {
    auto start = s.position;
    bool successes = false;

    for (;;) {
        const auto& result = apply(p, s);

        if (!result) {
            if constexpr (FailOnNoSuccess) {
                if (!successes) {
                    return s.return_fail();
                }
            }
            break;
        }
        successes = true;

        if constexpr (!std::is_empty_v<std::decay_t<Fun>>) {
            f(std::move(*result)); // We're done with the result here so we can move it.
        }

        if constexpr (!std::is_empty_v<std::decay_t<Sep>>) {
            if (!apply(sep, s)) break;
        }
    }

    return s.return_success(s.convert(start, s.position));
}


template <typename State, typename Parser>
inline constexpr auto times(State& s, size_t n, Parser p) {
    auto start = s.position;
    for (size_t i = 0; i < n; ++i) {
        if (const auto& result = apply(p, s); !result) return s.return_fail_result_default(result);
    }
    return s.return_success(s.convert(start, s.position));
}

/**
 * Recursive helper for `get_parsed`
 */
template <typename State, typename Iterator, typename Parser, typename... Parsers>
inline constexpr auto get_parsed_recursive(State& s, Iterator original_position, Parser p, Parsers... ps) {
    if (const auto& result = apply(p, s)) {
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
inline constexpr auto lift_or_rec(State& s, F f, Parser p, Parsers... ps) {
    auto start_pos = s.position;
    if (const auto& result = apply(p, s)) {
        return s.return_success(f(std::move(*result)));
    } else {
        s.position = start_pos;
        if constexpr (sizeof...(ps) > 0) {
            return lift_or_rec(s, f, ps...);
        } else {
            // All parsers failed
            using result_type = std::decay_t<decltype(f(*result))>;
            return s.template return_fail_change_result<result_type>(result);
        }
    }
}

}

#endif // PARSER_COMBINATORS_INTERNAL_H
