#ifndef PARSIMON_INTERNAL_MONAD_INTERNAL_H
#define PARSIMON_INTERNAL_MONAD_INTERNAL_H

#include <utility>
#include "parsimon/core.h"

namespace parsimon::internal {

/**
 * Currying of an arbitrary function
 */
template <std::size_t num_args, typename Fn>
inline constexpr auto curry_n(Fn f) {
    if constexpr (num_args > 1) {
        return [f](auto&& v) {
            using v_type = decltype(v);
            return curry_n<num_args-1>([&v, f](auto&&...vs) {
                return f(std::forward<v_type>(v), std::forward<decltype(vs)>(vs)...);
            });
        };
    } else {
        return f;
    }
}

template <typename ResultType, typename State, typename Fn, typename Parser, typename... Ps>
inline constexpr auto lift_internal(State& s, Fn f, Parser p, Ps... ps) {
    if (auto&& res = apply(p, s)) {
        if constexpr (sizeof...(ps) == 0) {
            return f(*std::forward<decltype(res)>(res));
        } else {
            return lift_internal<ResultType>(s, f(*std::forward<decltype(res)>(res)), ps...);
        }
    } else {
        return s.template return_fail_change_result<ResultType>(res);
    }
}

/**
 * Intermediate step for lifting
 */
template <typename Fn, typename... Parsers>
inline constexpr auto lift_prepare(Fn f, Parsers... ps) {
    return parser([=](auto& s) {
        types::assert_functor_application_modify<decltype(s), Fn, decltype(s), Parsers...>();
        using result_type = std::decay_t<decltype(*f(s, *apply(ps, s)...))>;
        if constexpr (sizeof...(Parsers) == 0) {
            return f(s);
        } else {
            return lift_internal<result_type>(s, curry_n<sizeof...(Parsers) + 1>(f)(s), ps...);
        }
    });
}

/**
 * Recursive bind
 */
template <typename Fn, typename Parser, typename... Parsers>
inline constexpr auto bind_internal(Fn f, Parser p, Parsers... ps) {
    return p >>= [=](auto&& r) {
        if constexpr (sizeof...(Parsers) == 0) {
            return f(std::forward<decltype(r)>(r));
        } else {
            return bind_internal(f(std::forward<decltype(r)>(r)), ps...);
        }
    };
}

}

#endif // PARSIMON_INTERNAL_MONAD_INTERNAL_H
