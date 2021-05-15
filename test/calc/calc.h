#ifndef CALC_H
#define CALC_H

#include <sstream>
#include "anpa/anpa.h"


template <typename T>
constexpr auto const_pow(T a, T b) {
    T result = 1;
    for (int i = 0; i < b; ++i) result *= a;
    return result;
}

// A parser for arithmetic expressions. It does not support any whitespace.
constexpr auto expr = anpa::recursive<int>([](auto p) {
    using namespace anpa;
    constexpr auto ops = [](auto c) {
        return [c](auto a, auto b) {
            switch (c) {
            case '+': return a + b;
            case '-': return a - b;
            case '*': return a * b;
            case '/': return a / b;
            case '^': return const_pow(a, b);
            default: return 0; // This can never happen
            }
        };
    };

    constexpr auto addOp = lift(ops, item<'+'>() || item<'-'>());
    constexpr auto mulOp = lift(ops, item<'*'>() || item<'/'>());
    constexpr auto expOp = lift(ops, item<'^'>());

    auto atom = integer() || (item<'('>() >> p << item<')'>());
    auto exp = chain(atom, expOp);
    auto factor = chain(exp, mulOp);
    return chain(factor, addOp);
});

#endif //CALC_H
