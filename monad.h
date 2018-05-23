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
    return m1 >>= [=] (...) {
        return m2;
    };
}

/*
 * Put the provided value in the monad on successful computation
 */
template <typename Monad, typename Value>
inline constexpr auto operator>=(Monad m, Value v) {
    return m >>= [m, v] (...) {
        return Monad::mreturn(v);
    };
}

/*
 * Combine two monads, ignoring the result of the second one
 */
template <typename Monad1, typename Monad2>
inline constexpr auto operator<<(Monad1 m1, Monad2 m2) {
    return m1 >>= [=] (auto&& r) {
        return m2 >>= [=](...) {
            return std::decay_t<Monad1>::mreturn(r);
        };
    };
}

namespace monad {

/**
 * Recursive compile time resolver for lift methods
 * T type is only used if we are emplacing a value in the monad, and f is only
 * used if we are not, i.e. a normal function lift.
 */
template <typename T, bool Emplace, typename F, typename Monad, size_t total, typename Arg, typename... Args>
inline constexpr auto lift_inner2([[maybe_unused]] F f, Arg&& arg, Args&&... args) {
    if constexpr (total == 0) {
        if constexpr (Emplace)
            return Monad::template mreturn_forward<T>(std::forward<Arg>(arg), std::forward<Args>(args)...);
        else
            return Monad::mreturn(f(std::forward<Arg>(arg), std::forward<Args>(args)...));
    } else {
        return arg >>= [=](auto &&v) {
            return lift_inner2<T, Emplace, F, Monad, total - 1>(f, args..., v);
        };
    }
}

/**
 * Middle step to do parameter counting
 */
template <typename T, bool Emplace, typename F, typename Monad, typename... Monads>
inline constexpr auto lift_inner(F f, Monad&& m, Monads&&... monads) {
    return lift_inner2<T, Emplace, F, std::decay_t<Monad>, sizeof...(Monads) + 1>(f, std::forward<Monad>(m), std::forward<Monads>(monads)...);
}

/**
 * Apply a function f to the results of the monads (evaluated left to right) and
 * put the result in the monad
 */
template <typename F, typename... Monads>
inline constexpr auto lift(F f, Monads&&... monads) {
    return lift_inner<bool, false>(f, std::forward<Monads>(monads)...);
}

/**
 * Apply a function f to the results of the monads (evaluated left to right) and
 * put the result as a lazy value in the monad
 */
template <typename F, typename... Monads>
inline constexpr auto lift_lazy(F f, Monads&&... monads) {
    return lift_inner<bool, false>(lazy::make_lazy_forward_fun(f), std::forward<Monads>(monads)...);
}

/**
 * Create an object by passing the results of the monads (evaluated left to right)
 * to its constructer, then put the object in the monad.
 * This is a specialized version of lift that avoids unnecessary copying by constructing
 * the object in place.
 */
template <typename T, typename... Monads>
inline constexpr auto lift_value(Monads&&... monads) {
    //
    return lift_inner<T, true>([](){}, std::forward<Monads>(monads)...);
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
