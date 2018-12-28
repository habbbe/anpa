#ifndef MONAD_H
#define MONAD_H

#include <type_traits>
#include <utility>
#include "lazy.h"

/*
 * Combine two monads, ignoring the result of the first one
 */
template <typename Monad1, typename Monad2>
inline constexpr auto operator>>(Monad1 m1, Monad2 m2) {
    return m1 >>= [=] (const auto &) {
        return m2;
    };
}

/*
 * Put the provided value in the monad on successful computation
 */
template <typename Monad, typename Value>
inline constexpr auto operator>=(Monad m, Value &&v) {
    return m >> Monad::mreturn(std::forward<Value>(v));
}

/*
 * Combine two monads, ignoring the result of the second one
 */
template <typename Monad1, typename Monad2>
inline constexpr auto operator<<(Monad1 m1, Monad2 m2) {
    return m1 >>= [=] (auto&& r) {
        return m2 >>= [=](const auto &) {
            return std::decay_t<Monad1>::mreturn(r);
        };
    };
}

namespace monad {

/**
 * Currying of an arbitrary function
 */
template <std::size_t num_args, typename F>
inline constexpr auto curry_n(F f) {
    if constexpr (num_args > 0) {
        return [=](auto &&v) {
            return curry_n<num_args-1>([&](auto&&...vs) {
                return f(std::forward<decltype(v)>(v), std::forward<decltype(vs)>(vs)...);
            });
        };
    } else {
        return f();
    }
}

/**
 * Recursive lifting of functions
 */
template <typename F, typename M, typename... Ms>
inline constexpr auto lift_internal(F f, M m, Ms... ms) {
    return m >>= [=](auto &&r) {
        if constexpr (sizeof...(ms) == 0) {
            return f(std::forward<decltype(r)>(r));
        } else {
            return lift_internal(f(std::forward<decltype(r)>(r)), ms...);
        }
    };
}

/**
 * Intermediate step for lifting
 */
template <typename F, typename... Ms>
inline constexpr auto lift_prepare(F f, Ms... ms) {
    return lift_internal(curry_n<sizeof...(Ms)>(f), ms...);
}

/**
 * Apply a function f to the results of the monads (evaluated left to right) and
 * put the result in the monad
 */
template <typename F, typename M, typename... Ms>
inline constexpr auto lift(F&& f, M m, Ms... ms) {
    auto fun = [=](auto &&...ps) {
        return M::mreturn(f(std::forward<decltype(ps)>(ps)...));
    };
    return lift_prepare(fun, m, ms...);
}

/**
 * Apply a function f to the results of the monads (evaluated left to right) and
 * put the result as a lazy value in the monad
 */
template <typename F, typename... Monads>
inline constexpr auto lift_lazy(F f, Monads&&... monads) {
    return lift(lazy::make_lazy_forward_fun(f), std::forward<Monads>(monads)...);
}

/**
 * Create an object by passing the results of the monads (evaluated left to right)
 * to its constructor, then put the object in the monad.
 * This is a specialized version of lift that avoids unnecessary copying by constructing
 * the object in place.
 */
template <typename T, typename M, typename... Ms>
inline constexpr auto lift_value(M m, Ms... ms) {
    constexpr auto fun = [](auto &&...ps) {
        return M::template mreturn_forward<T>(std::forward<decltype(ps)>(ps)...);
    };
    return lift_prepare(fun, m, ms...);
}

/**
 * Create an object by passing the lazy results of the monads (evaluated left to right)
 * to its constructor, then put it as a lazy value in the monad.
 */
template <typename T, typename... Monads>
inline constexpr auto lift_value_lazy(Monads&&... monads) {
    return lift(lazy::make_lazy_value_forward_fun<T>(), std::forward<Monads>(monads)...);
}

/**
 * Create an object by passing the non-lazy results of the monads (evaluated left to right)
 * to its constructer, then put it as a lazy value in the monad.
 */
template <typename T, typename... Monads>
inline constexpr auto lift_value_lazy_raw(Monads&&... monads) {
    return lift(lazy::make_lazy_value_forward_fun_raw<T>(), std::forward<Monads>(monads)...);
}

}


#endif // MONAD_H
