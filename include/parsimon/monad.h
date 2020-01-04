#ifndef PARSIMON_MONAD_H
#define PARSIMON_MONAD_H

#include <type_traits>
#include <utility>
#include <memory>
#include "parsimon/internal/monad_internal.h"
#include "parsimon/core.h"

namespace parsimon {

/**
 * Combine two monads, ignoring the result of the first one
 */
template <typename P1, typename P2>
inline constexpr auto operator>>(parser<P1> p1, parser<P2> p2) {
    return p1 >>= [=](auto&&) {
        return p2;
    };
}

/**
 * Put the provided value `v' in the parser monad on successful computation of `p`.
 */
template <typename P, typename Value>
inline constexpr auto operator>=(parser<P> p, Value&& v) {
    return p >> mreturn(std::forward<Value>(v));
}

/**
 * Combine two parsers, returning the result of the first one.
 *
 * This is an optimized implementation. Using
 * @code
 * p1 >>= [=](auto&& r) { return p2 >> parser<P1>::mreturn(std::forward<decltype(r)>(r)); }
 * @endcode
 * works, but doesn't optimize nearly as well.
 */
template <typename P1, typename P2>
inline constexpr auto operator<<(parser<P1> p1, parser<P2> p2) {
    return parser([=](auto& s) {
        auto result = apply(p1, s);
        if (result) {
            if (auto result2 = apply(p2, s)) {
                return result;
            } else {
                return s.template return_fail_change_result<std::decay_t<decltype(*result)>>(result2);
            }
        }
        return result;
    });
}

/**
 * Monadic bind for arbitrary number of parsers. Use this to reduce indentation
 * level if you need to evaluate multiple monads and use all results
 * in the same context.
 * Example:
 * @code
 * parsimon::bind([](auto&& a, auto&& b, auto&& c) {
 *     do_something_with(a, b, c);
 *     return some_new_monad;
 * }, p1, p2, p3);
 * @endcode
 */
template <typename F, typename... Parsers>
inline constexpr auto bind(F f, Parsers... ps) {
    types::assert_parsers_not_empty<Parsers...>();
    return internal::bind_internal(internal::curry_n<sizeof...(Parsers)>(f), ps...);
}

/**
 * Apply a function `f` to the results of the parsers (evaluated left to right) and
 * put the result in the monad.
 *
 * If the return value of `f` is `void`, then `none` will be used as the result.
 *
 * It's also possible to pass zero parsers to lift an object (even non-copyable)
 * to the parser monad by having `f` returning it.
 *
 * @param f a functor with the following signature:
 *            `ReturnType(auto&&... results)`
 */
template <typename F, typename... Parsers>
inline constexpr auto lift(F f, Parsers... ps) {
    auto fun = [f](auto& s, auto&&... rs) {
        if constexpr (!types::has_arg<F>) {
            return s.template return_success_emplace<none>();
        } else if constexpr (std::is_void_v<decltype(f(std::forward<decltype(rs)>(rs)...))>) {
            f(std::forward<decltype(rs)>(rs)...);
            return s.template return_success_emplace<none>();
        } else {
            return s.template return_success(f(std::forward<decltype(rs)>(rs)...));
        }
    };
    return internal::lift_prepare(fun, ps...);
}

/**
 * Create a value by passing the results of the parsers (evaluated left to right)
 * to its constructor, then put the object in the parser monad.
 *
 * This is a specialized version of parsimon::lift that avoids unnecessary copying/moving
 * by constructing the object in place.
 */
template <typename T, typename... Parsers>
inline constexpr auto lift_value(Parsers... ps) {
    return internal::lift_prepare([](auto& s, auto&&... rs) {
        return s.template return_success_emplace<T>(std::forward<decltype(rs)>(rs)...);
    }, ps...);
}

}


#endif // PARSIMON_MONAD_H
