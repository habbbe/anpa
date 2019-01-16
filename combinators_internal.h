#ifndef PARSER_COMBINATORS_INTERNAL_H
#define PARSER_COMBINATORS_INTERNAL_H

#include <type_traits>
#include <utility>
#include <tuple>
#include <valgrind/callgrind.h>
#include "types.h"

namespace parse::internal {

/**
 * General helper for evaluating a parser multiple times with an optional separator.
 */
template <bool FailOnNoSuccess = false,
          typename State,
          typename Parser,
          typename Fun = none,
          typename Sep = none,
          typename Break = none,
          bool Eat = true,
          bool Include = false
          >
inline constexpr auto many(State &s,
                           Parser p,
                           [[maybe_unused]] Fun f = {},
                           [[maybe_unused]] Sep sep = {},
                           [[maybe_unused]] Break breakOn = {}) {
    auto start = s.position;
    bool successes = false;

    for (;;) {
        if constexpr (!std::is_same_v<none, std::decay_t<Break>>) {
            auto p = s.position;
            if (apply(breakOn, s)) {
                if constexpr (!Eat) s.set_position(p);
                return s.return_success(s.convert(start, Include ? s.position : p));
            } else {
                s.position = p;
            }
        }

        auto res = apply(p, s);

        if (!res) {
            if constexpr (FailOnNoSuccess) {
                if (!successes) {
                    return s.return_fail();
                }
            }
            break;
        }
        successes = true;

        if constexpr (!std::is_same_v<none, std::decay_t<Fun>>) {
            f(std::move(*res)); // We're done with the result here so we can move it.
        }

        if constexpr (!std::is_same_v<none, std::decay_t<Sep>>) {
            if (!apply(sep, s)) break;
        }
    }

    return s.return_success(s.convert(start, s.position));
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
    auto startPos = s.position;
    if (auto result = apply(p, s)) {
        if constexpr (std::is_same_v<none, std::decay_t<F>>) {
            return s.return_success(std::move(*result));
        } else {
            return s.return_success(f(std::move(*result)));
        }
    } else if constexpr (sizeof...(ps) > 0) {
        s.position = startPos;
        return lift_or_rec(s, f, ps...);
    } else {
        // All parsers failed
        s.position = startPos;
        if constexpr (std::is_same_v<none, std::decay_t<F>>) {
            return s.template return_fail_result(result);
        } else {
            using result_type = std::decay_t<decltype(f(*result))>;
            return s.template return_fail_change_result<result_type>(result);
        }
    }
}

}

#endif // PARSER_COMBINATORS_INTERNAL_H
