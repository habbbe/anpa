#ifndef PARSIMON_INTERNAL_COMBINATORS_INTERNAL_H
#define PARSIMON_INTERNAL_COMBINATORS_INTERNAL_H

#include <type_traits>
#include <utility>
#include <valgrind/callgrind.h>
#include "parsimon/types.h"
#include "parsimon/monad.h"
#include "parsimon/options.h"


namespace parsimon::internal {

/**
 * General helper for evaluating a parser multiple times with an optional separator.
 */
template <options Options,
          typename State,
          typename Fn = no_arg,
          typename Sep = no_arg,
          typename... Parsers>
inline constexpr auto many_internal(State& s,
                            Fn f,
                            [[maybe_unused]] Sep sep,
                            Parsers... ps ) {
    auto start = s.position;
    bool successes = false;
    constexpr bool no_trailing_sep = types::has_arg<Sep> && has_options(Options, options::no_trailing_separator);

    for (;;) {
        if (auto&& result = apply(lift(f, ps...), s); !result) {
            if constexpr (has_options(Options, options::fail_on_no_parse)) {
                if (!successes) {
                    return s.return_fail_result_default(result);
                }
            }
            if constexpr (no_trailing_sep) {
                if (successes) {
                    return s.return_fail_result_default(result);
                }
            }
            break;
        }

        if constexpr (has_options(Options, options::fail_on_no_parse) || no_trailing_sep) successes = true;

        if constexpr (!std::is_empty_v<Sep>) {
            if (!apply(sep, s)) break;
        }
    }

    return s.return_success(s.convert(start, s.position));
}

template <options Options,
          typename State,
          typename Init = no_arg,
          typename Fn,
          typename Acc,
          typename ParserSep,
          typename... Parsers>
inline constexpr auto fold_internal(State& s,
                                    Init init,
                                    Fn f,
                                    Acc acc,
                                    ParserSep sep,
                                    Parsers... ps) {
    if constexpr (types::has_arg<Init>) init(acc);
    auto result = many_internal<Options>(s, [&acc, f](auto&&... rs) {
        if constexpr (has_options(Options, options::replace)) {
            acc = std::move(f(std::move(acc), std::forward<decltype(rs)>(rs)...));
        } else {
            f(acc, std::forward<decltype(rs)>(rs)...);
        }
    }, sep, ps...);

    if constexpr (has_options(Options, options::fail_on_no_parse)
            || (has_options(Options, options::no_trailing_separator) && types::has_arg<ParserSep>)) {
        if (!result) {
            return s.template return_fail_change_result<Acc>(result);
        }
    }
    return s.return_success(std::move(acc));
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
template <typename State, typename InputIt, typename Parser, typename... Parsers>
inline constexpr auto get_parsed_recursive(State& s, InputIt original_position, Parser p, Parsers... ps) {
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
template <typename State, typename InputIt, typename Fn, typename Parser, typename... Parsers>
inline constexpr auto lift_or_rec(State& s, InputIt start_pos, Fn f, Parser p, Parsers... ps) {
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
