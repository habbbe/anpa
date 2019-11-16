#ifndef MONAD_H
#define MONAD_H

#include <type_traits>
#include <utility>
#include <memory>
#include "lazy.h"
#include "core.h"

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
 * works, but doesn't optmize nearly as well.
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
 * Currying of an arbitrary function
 */
template <std::size_t num_args, typename F>
inline constexpr auto curry_n(F f) {
    if constexpr (num_args > 1) {
        return [=](auto&& v) {
            return curry_n<num_args-1>([&](auto&&...vs) {
                return f(std::forward<decltype(v)>(v), std::forward<decltype(vs)>(vs)...);
            });
        };
    } else {
        return f;
    }
}

/**
 * Recursive lifting of functions
 */
template <typename F, typename P, typename... Ps>
inline constexpr auto lift_internal(F f, P p, Ps... ps) {
    return p >>= [=](auto&& r) {
        if constexpr (sizeof...(ps) == 0) {
            return f(std::forward<decltype(r)>(r));
        } else {
            return lift_internal(f(std::forward<decltype(r)>(r)), ps...);
        }
    };
}

/**
 * Intermediate step for lifting
 */
template <typename F, typename... Ps>
inline constexpr auto lift_prepare(F f, Ps... ps) {
    return lift_internal(curry_n<sizeof...(Ps)>(f), ps...);
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
    return lift_prepare(fun, p, ps...);
}

/**
 * Apply a function f to the results of the parsers (evaluated left to right) and
 * put the result as a lazy value in the parser monad
 */
template <typename F, typename... Parsers>
inline constexpr auto lift_lazy(F f, Parsers... ps) {
    return lift(lazy::make_lazy_forward_fun(f), std::forward<Parsers>(ps)...);
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
    return lift_prepare(fun, p, ps...);
}

/**
 * Create an object by passing the lazy results of the monads (evaluated left to right)
 * to its constructor, then put it as a lazy value in the monad.
 */
template <typename T, typename... Parsers>
inline constexpr auto lift_value_lazy(Parsers... ps) {
    return lift(lazy::make_lazy_value_forward_fun<T>(), std::forward<Parsers>(ps)...);
}

/**
 * Create an object by passing the non-lazy results of the monads (evaluated left to right)
 * to its constructer, then put it as a lazy value in the monad.
 */
template <typename T, typename... Parsers>
inline constexpr auto lift_value_lazy_raw(Parsers... ps) {
    return lift(lazy::make_lazy_value_forward_fun_raw<T>(), std::forward<Parsers>(ps)...);
}

template <typename Parser>
inline constexpr auto lift_shared(Parser p) {
    return lift([](auto&& s){
        using type = std::decay_t<decltype(s)>;
        return std::make_shared<type>(std::forward<decltype(s)>(s));
    }, p);
}

template <typename Parser>
inline constexpr auto lift_lazy(Parser p) {
    return lift([](auto&& s) {
        return lazy::make_lazy(std::forward<decltype(s)>(s));
    }, p);
}

}


#endif // MONAD_H
