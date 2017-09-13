#ifndef LAZY_VALUE_H
#define LAZY_VALUE_H

namespace lazy {

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

template <typename F, typename LazyValue>
constexpr auto apply(F f, LazyValue v) {
    return value([=]() {
        return f(v());
    });
}

template <typename Get>
struct value {
    Get get;
    constexpr value(Get&& get) : get{std::forward<Get>(get)} {}

    constexpr auto operator()() const {
        return get();
    }

    template <typename LazyValue, typename F>
    static constexpr auto modify(LazyValue v, F f) {
        return lazy::modify(v, f);
    }
};

template <typename T>
inline auto constexpr make_lazy(T&& t) {
    return value([=]() {
        return t;
    });
}

template <typename F, typename... Args>
inline auto constexpr make_lazy_forward(F f, Args&&... args) {
    return value([=] () {
        return f(args...);
    });
}

template <typename T, typename... Args>
inline auto constexpr make_lazy_value_forward(Args&&... args) {
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

template <typename LazyVal, typename F>
inline auto constexpr lazy_apply(LazyVal val, F f) {
    return value([=] () {
        return f(val());
    });
}

}
#endif // LAZY_VALUE_H
