#ifndef LAZY_VALUE_H
#define LAZY_VALUE_H

#include <utility>

namespace lazy {

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
inline auto constexpr make_lazy(T t) {
    return value([=]() {
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

}
#endif // LAZY_VALUE_H
