#ifndef LAZY_VALUE_H
#define LAZY_VALUE_H

#include <utility>
#include "parsimon/monad.h"
#include "parsimon/internal/combinators_internal.h"

namespace parsimon::lazy {

#define inline_lazy(x) []() {return x;}

template <typename F>
struct value;


template <typename LazyValue, typename F>
constexpr auto modify(LazyValue v, F f) {
    return value([=]() {
        auto val = v();
        f(val);
        return val;
    });
}

template <typename LazyFun, typename... LazyVals>
inline auto constexpr apply(LazyFun&& f, LazyVals&&... vals) {
    return value([=] () {
        return f()(vals()...);
    });
}

template <typename Get>
struct value {

    using is_lazy = bool;
    Get get;
    constexpr value(Get&& get) : get{std::forward<Get>(get)} {}

    constexpr auto operator()() const {
        return get();
    }

    template <typename LazyValue, typename F>
    static constexpr auto modify(LazyValue v, F f) {
        return lazy::modify(v, f);
    }

    template <typename LazyFun, typename... LazyVals>
    inline auto constexpr apply(LazyFun&& f, LazyVals&&... vals) {
        lazy::apply(f, vals...);
    }
};

template <typename T>
inline auto constexpr make_lazy(T&& t) {
    return value([t = std::forward<T>(t)]() {
        return t;
    });
}

template <typename F, typename... Args>
inline auto constexpr make_lazy_forward(F f, Args&&... args) {
    return value([=] () {
        return f(args()...);
    });
}

template <typename F, typename... Args>
inline auto constexpr make_lazy_forward_raw(F f, Args&&... args) {
    return value([=] () {
        return f(args...);
    });
}

template <typename T, typename... Args>
inline auto constexpr make_lazy_value_forward(Args&&... args) {
    return value([=] () {
        return T(args()...);
    });
}

template <typename T, typename... Args>
inline auto constexpr make_lazy_value_forward_raw(Args&&... args) {
    return value([=] () {
        return T(args...);
    });
}

template <typename F>
inline auto constexpr make_lazy_forward_fun(F f) {
    return [=] (auto&&... args) {
        return make_lazy_forward(f, args...);
    };
}

template <typename T>
inline auto constexpr make_lazy_value_forward_fun() {
    return [=] (auto&&... args) {
        return make_lazy_value_forward<T>(args...);
    };
}

template <typename T>
inline auto constexpr make_lazy_value_forward_fun_raw() {
    return [=] (auto&&... args) {
        return make_lazy_value_forward_raw<T>(args...);
    };
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

/**
 * Apply a function f to the results of the parsers (evaluated left to right) and
 * put the result as a lazy value in the parser monad
 */
template <typename F, typename... Parsers>
inline constexpr auto lift_lazy(F f, Parsers... ps) {
    return lift(lazy::make_lazy_forward_fun(f), std::forward<Parsers>(ps)...);
}

/**
 * Lift a type to the parser monad after applying the first successful parser's
 * result to its constructor. The constructor must provide an overload for every
 * parser result type.
 * This version applies the constructor to a lazy argument
 */
template <typename T, typename Parser, typename... Parsers>
inline constexpr auto lift_or_value_from_lazy(Parser p, Parsers... ps) {
    return parser([=](auto& s) {
        constexpr auto construct = [](auto arg) {
            return T(arg());
        };
        return internal::lift_or_rec(s, construct, p, ps...);
    });
}

}
#endif // LAZY_VALUE_H
