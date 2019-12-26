#ifndef MONAD_H
#define MONAD_H

#include <type_traits>
#include <utility>
#include <memory>
#include "parsimon/internal/monad_internal.h"
#include "parsimon/core.h"

namespace parsimon {

/**
 * Combine two monads, ignoring the result of the first one
 */
template <typename Parser1, typename Parser2>
inline constexpr auto operator>>(Parser1 p1, Parser2 p2) {
    return p1 >>= [=](auto&&) {
        return p2;
    };
}

/**
 * Put the provided value `v' in the parser monad on successful computation of `p`.
 */
template <typename Parser, typename Value>
inline constexpr auto operator>=(Parser p, Value&& v) {
    return p >> Parser::mreturn(std::forward<Value>(v));
}

/**
 * Combine two parsers, ignoring the result of the second one.
 * This is an optimized version. Using
 * `p1 >>= [=](auto&& r) { return p2 >> Parser1::mreturn(std::forward<decltype(r)>(r)); }`
 * works, but doesn't optimize nearly as well.
 */
template <typename Parser1, typename Parser2>
inline constexpr auto operator<<(Parser1 p1, Parser2 p2) {
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
 * Monadic bind for multiple monads. Use this to reduce indentation
 * level if you need to evaluate multiple monads and use all results
 * in the same context.
 * Example:
 * parsimon::bind([](auto&& a, auto&& b, auto&& c) {
 *     do_something_with(a, b, c);
 *     return some_new_monad;
 * }, p1, p2, p3);
 */
template <typename F, typename Parser, typename... Parsers>
inline constexpr auto bind(F f, Parser p, Parsers... ps) {
    return lift_prepare(f, p, ps...);
}

/**
 * Apply a function f to the results of the monads (evaluated left to right) and
 * put the result in the monad
 */
template <typename F, typename Parser, typename... Parsers>
inline constexpr auto lift(F f, Parser p, Parsers... ps) {
    auto fun = [=](auto&&... ps) {
        return Parser::mreturn(f(std::forward<decltype(ps)>(ps)...));
    };
    return internal::lift_prepare(fun, p, ps...);
}

/**
 * Create an object by passing the results of the monads (evaluated left to right)
 * to its constructor, then put the object in the monad.
 * This is a specialized version of lift that avoids unnecessary copying by constructing
 * the object in place.
 */
template <typename T, typename Parser, typename... Parsers>
inline constexpr auto lift_value(Parser p, Parsers... ps) {
    constexpr auto fun = [](auto&&... ps) {
        return Parser::template mreturn_emplace<T>(std::forward<decltype(ps)>(ps)...);
    };
    return internal::lift_prepare(fun, p, ps...);
}

}


#endif // MONAD_H
