#ifndef PARSER_COMBINATORS_INTERNAL_H
#define PARSER_COMBINATORS_INTERNAL_H

#include <type_traits>
#include <utility>
#include <valgrind/callgrind.h>

namespace parse::internal {

/**
 * General helper for evaluating a parser multiple times with an optional separator.
 */
template <typename State,
          typename Parser,
          typename Fun = std::nullptr_t,
          typename Sep = std::nullptr_t,
          typename Break = std::nullptr_t,
          bool Eat = true,
          bool Include = false>
inline constexpr auto many(State &s,
                           Parser p,
                           Fun f = nullptr,
                           Sep sep = nullptr,
                           Break breakOn = nullptr) {
    auto start = s.position;

    for (auto res = apply(p, s); res; res = apply(p, s)) {

        if constexpr (!std::is_null_pointer_v<std::decay_t<Break>>) {
            auto p = s.position;
            if (apply(breakOn, s)) {
                if constexpr (!Eat) s.set_position(p);
                return s.return_success(s.convert(start, Include ? s.position : p));
            }
        }

        if constexpr (!std::is_null_pointer_v<std::decay_t<Fun>>) {
            f(*res);
        }

        if constexpr (!std::is_null_pointer_v<std::decay_t<Sep>>) {
            if (!apply(sep, s)) break;
        }
    }
    return s.return_success(s.convert(start, s.position));
}


/**
 * General helper for evaluating a parser multiple times with an optional separator.
 * This seems to optimize better on GCC than the above in some cases.
 */
template <typename Parser,
          typename Fun = std::nullptr_t,
          typename Sep = std::nullptr_t,
          typename Break = std::nullptr_t,
          bool Eat = true,
          bool Include = false>
inline constexpr auto many_2(Parser p,
                           Fun f = nullptr,
                           Sep sep = nullptr,
                           Break breakOn = nullptr) {
    return parser([=](auto &s) {
        auto start = s.position;

        while (true) {

            if constexpr (!std::is_null_pointer_v<std::decay_t<Break>>) {
                auto p = s.position;
                if (apply(breakOn, s)) {
                    if constexpr (!Eat) s.set_position(p);
                    return s.return_success(s.convert(start, Include ? s.position : p));
                }
            }

            auto res = apply(p, s);

            if (!res) break;

            if constexpr (!std::is_null_pointer_v<std::decay_t<Fun>>) {
                f(s, *res);
            }

            if constexpr (!std::is_null_pointer_v<std::decay_t<Sep>>) {
                if (!apply(sep, s)) break;
            }
        }
        return s.return_success(s.convert(start, s.position));
    });
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
