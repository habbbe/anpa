#ifndef PARSIMON_INTERNAL_COMBINATORS_INTERNAL_H
#define PARSIMON_INTERNAL_COMBINATORS_INTERNAL_H

#include <type_traits>
#include <utility>
#include <valgrind/callgrind.h>
#include "parsimon/types.h"
#include "parsimon/monad.h"


namespace parsimon::internal {

/**
 * General helper for evaluating a parser multiple times with an optional separator.
 */
template <bool FailOnNoSuccess = false,
          typename State,
          typename Sep = no_arg,
          typename Fn = no_arg,
          typename... Parsers>
inline constexpr auto many(State& s,
                            [[maybe_unused]] Sep sep,
                            Fn f,
                            Parsers... ps ) {
    auto start = s.position;
    bool successes = false;

    for (;;) {
        if (auto&& result = apply(lift(f, ps...), s); !result) {
            if constexpr (FailOnNoSuccess) {
                if (!successes) {
                    return s.return_fail_result_default(result);
                }
            }
            break;
        }

        if constexpr (FailOnNoSuccess) successes = true;

        if constexpr (!std::is_empty_v<Sep>) {
            if (!apply(sep, s)) break;
        }
    }

    return s.return_success(s.convert(start, s.position));
}

template <typename Container,
          typename State,
          typename Init = no_arg,
          typename Inserter,
          typename ParserSep,
          typename... Parsers>
inline constexpr auto many_mutate_internal(State& s,
                                           Init init,
                                           Inserter inserter,
                                           ParserSep sep,
                                           Parsers... ps) {
    Container c{};
    if constexpr (types::has_arg<Init>) init(c);
    many(s, sep, [&c, inserter](auto&&... rs) {
        inserter(c, std::forward<decltype(rs)>(rs)...);
    }, ps...);
    return s.return_success(std::move(c));
}

template <typename ProvidedArg, typename DefaultArg>
inline constexpr auto default_arg(ProvidedArg provided_arg, DefaultArg default_arg) {
    if constexpr (!types::has_arg<ProvidedArg>) {
        return default_arg;
    } else {
        return provided_arg;
    }
}

template <typename State, typename Size, typename Parser>
inline constexpr auto times(State& s, Size n, Parser p) {
    auto start = s.position;
    for (Size i = 0; i < n; ++i) {
        if (auto&& result = apply(p, s); !result) return s.return_fail_result_default(result);
    }
    return s.return_success(s.convert(start, s.position));
}

/**
 * Recursive helper for `get_parsed`
 */
template <typename State, typename Iterator, typename Parser, typename... Parsers>
inline constexpr auto get_parsed_recursive(State& s, Iterator original_position, Parser p, Parsers... ps) {
    if (auto&& result = apply(p, s)) {
        if constexpr (sizeof...(Parsers) == 0) {
            return s.return_success(s.convert(original_position, s.position));
        } else {
            return get_parsed_recursive(s, original_position, ps...);
        }
    } else {
        return s.template return_fail_result_default(result);
    }
}

// Compile time recursive resolver for lifting of arbitrary number of parsers
template <typename State, typename Iterator, typename Fn, typename Parser, typename... Parsers>
inline constexpr auto lift_or_rec(State& s, Iterator start_pos, Fn f, Parser p, Parsers... ps) {
    using result_type = decltype(f(std::move(*apply(p, s))));
    constexpr auto void_return = std::is_void_v<result_type>;
    if (auto&& result = apply(p, s)) {
        if constexpr (void_return) {
            f(*std::forward<decltype(result)>(result));
            return s.template return_success_emplace<empty_result>();
        } else {
            return s.return_success(f(*std::forward<decltype(result)>(result)));
        }
    } else {
        s.set_position(start_pos);
        if constexpr (sizeof...(ps) > 0) {
            return lift_or_rec(s, start_pos, f, ps...);
        } else {
            // All parsers failed
            using actual_result_type = std::conditional_t<void_return, empty_result, result_type>;
            return s.template return_fail_change_result<actual_result_type>(result);
        }
    }
}

}

#endif // PARSIMON_INTERNAL_COMBINATORS_INTERNAL_H
