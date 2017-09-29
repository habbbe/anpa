#ifndef PARSER_COMBINATORS_H
#define PARSER_COMBINATORS_H

#include <type_traits>
#include "parser_core.h"

namespace parse {

/**
 * Transform a parser to a parser that always succeeds.
 * Will return true as result if the parser succeeded, and false otherwise
 */
template <typename Parser>
inline constexpr auto succeed(Parser p) {
    return parser([=](auto &s) {
        if (has_result(p(s))) {
            return return_success(true);
        } else {
            return return_success(false);
        }
    });
}

/**
 * Transform a parser to a parser that fails on a successful, but empty result (as decided by std::empty
 * or == 0 if integral)
 */
template <typename Parser>
inline constexpr auto not_empty(Parser p) {
    return parser([=](auto &s) {
        auto result = p(s);
        using result_type = std::decay_t<decltype(get_result(result))>;
        constexpr auto empty = [](auto &t) {
            if constexpr (std::is_integral<result_type>::value) {
                return t == 0;
            } else {
                return std::empty(t);
            }
        };
        if (has_result(result) && !empty(get_result(result))) {
            return result;
        } else {
            using return_type = std::decay_t<decltype(get_result(result))>;
            return return_fail<return_type>();
        }
    });
}

/**
 * Make a parser that doesn't consume its input on failure
 */
template <typename Parser>
inline constexpr auto try_parser(Parser p) {
    return parser([=](auto &s) {
        // Make copy of rest of string to parse
        auto str_copy = s.rest;
        auto result = p(s);
        if (!has_result(result)) {
            s.rest = std::move(str_copy);
        }
        return result;
    });
}

/**
 * Make a parser non consuming
 */
template <typename Parser>
inline constexpr auto no_consume(Parser p) {
    return parser([=](auto &s) {
        auto str_copy = s.rest;
        auto result = p(s);
        s.rest = std::move(str_copy);
        return result;
    });
}

/**
 * Constrain a parser.
 * Takes in addition to a parser a predicate that takes the resulting type of the parser as argument.
 */
template <typename Predicate, typename Parser>
inline constexpr auto constrain(Predicate pred, Parser p) {
    return parser([=](auto &s) {
        auto result = p(s);
        if (has_result(result) && pred(get_result(result))) {
            return result;
        } else {
            return return_fail<decltype(get_result(result))>();
        }
    });
}

/**
 * Combine two parsers so that the second will be tried before failing.
 * If the two parsers return different types the return value will instead be `true`.
 */
template <typename Parser1, typename Parser2>
inline constexpr auto operator||(Parser1 p1, Parser2 p2) {
    return parser([=](auto &s) {
        using R1 = decltype(get_result(p1(s)));
        using R2 = decltype(get_result(p2(s)));
        if constexpr (std::is_same_v<R1, R2>) {
            auto result1 = p1(s);
            return has_result(result1) ? result1 : p2(s);
        } else {
            if (p1(s) || p2(s)) {
                return return_success(true);
            } else {
                return return_fail<bool>();
            }
        }
    });
}

/**
 * Modify the supplied user state
 */
template <typename Fun>
inline constexpr auto modify_state(Fun f) {
    return parser([=](auto &s) {
        using result_type = std::decay_t<decltype(f(s.user_state))>;
        if constexpr (std::is_void<result_type>::value) {
            f(s.user_state);
            return return_success(true);
        } else {
            return_success(f(s.user_state));
        }
    });
}

/**
 * Set a value in the state accessed by `Accessor` from the result of the parser.
 * Note: Make sure that a reference is returned by `acc`.
 */
template <typename Accessor, typename Parser>
inline constexpr auto set_in_state(Accessor acc, Parser p) {
    return parser([=](auto &s) {
        auto result = p(s);
        if (has_result(result)) {
            acc(s.user_state) = get_result(result);
        }
        return result;
    });
}

/**
 * Apply a provided function to the state after evaluating a number of parsers in sequence.
 * The function provided shall have the state as its first argument (by reference) and
 * then a number of arguments that matches the number of parsers (or variadic arguments)
 */
template <typename Fun, typename... Parsers>
inline constexpr auto apply_to_state(Fun f, Parsers...ps) {
    return parser([=](auto &s) {
        auto &state = s.user_state;
        auto to_apply = [f, &state] (auto&&...vals) {
            return f(state, std::forward<decltype(vals)>(vals)...);
        };
        return monad::lift(to_apply, ps...)(s);
    });
}

/**
 * Emplace a value in the state with `emplace` with the results of a number of parsers evaluated in sequence
 * to the user supplied state accessed by `acc`.
 */
template <typename Accessor, typename... Parsers>
inline constexpr auto emplace_to_state(Accessor acc, Parsers... ps) {
    return apply_to_state([acc](auto &s, auto&&...args) {return acc(s).emplace(std::forward<decltype(args)>(args)...);}, ps...);
}

/**
 * Emplace a value in the state with `emplace` with the results of a number of parsers evaluated in sequence
 * to the user supplied state.
 */
template <typename... Parsers>
inline constexpr auto emplace_to_state_direct(Parsers... ps) {
    return emplace_to_state([](auto &s) -> auto& {return s;}, ps...);
}

/**
 * Emplace a value in the state with `emplace_back` with the results of a number of parsers evaluated in sequence
 * to the user supplied state accessed by `acc`.
 */
template <typename Accessor, typename... Parsers>
inline constexpr auto emplace_back_to_state(Accessor acc, Parsers... ps) {
    return apply_to_state([acc](auto &s, auto&&...args) {return acc(s).emplace_back(std::forward<decltype(args)>(args)...);}, ps...);
}

/**
 * Emplace a value in the state with `emplace_back` with the results of a number of parsers evaluated in sequence
 * to the user supplied state.
 */
template <typename... Parsers>
inline constexpr auto emplace_back_to_state_direct(Parsers... ps) {
    return emplace_back_to_state([](auto &s) -> auto& {return s;}, ps...);
}

/**
 * Create a parser that applies a parser until it fails or until the result doesn't satisfy
 * the user provided criteria.
 */
template <typename Container, typename Parser>
inline constexpr auto many(Parser p) {
    return parser([=](auto &s) {
        Container c;
        for (auto result = p(s); has_result(result); result = p(s)) {
            c.push_back(get_result(result));
        }
        return return_success(c);
    });
}

/**
 * Create a parser that applies a parser multiple times and returns the number it succeeds.
 * Useful when applying parses to state.
 */
template <typename Parser>
inline constexpr auto many_simple(Parser p) {
    return parser([=](auto &s) {
        size_t successes = 0;
        while (has_result(p(s))) { ++successes; }
        return return_success(successes);
    });
}

/**
 * Create a parser that applies a parser until it fails and stores the result in
 * a container in the user provided state which is accessed by the function `acc`.
 * The number of added elements is then returned.
 * Note that this parser always succeeds, so you need to check the resulting number whether a
 * parse succeeded or not (or use parse::not_empty).
 * Note: The container in the state must have an `emplace_back` function.
 */
template <typename Accessor, typename Parser>
inline constexpr auto many_to_state(Accessor acc, Parser p) {
    return parser([=](auto &s) {
        size_t number_of_results = 0;
        auto &c = acc(s.user_state);
        for (auto result = p(s); has_result(result); result = p(s)) {
            c.emplace_back(std::move(get_result(result)));
            ++number_of_results;
        }
        return return_success(number_of_results);
    });
}

/**
 * Create a parser that applies a parser until it fails and stores the result in the user provided state.
 * Note that the user provided state needs to be a container.
 */
template <typename Predicate, typename Parser>
inline constexpr auto many_to_state(Parser p) {
    return many_to_state([](auto &s) -> auto& {return s;}, p);
}

/**
 * Create a parser that applies a parser until it fails and stores the result in the user
 * provided state.
 * Note that the user provided state needs to be a container.
 */
template <typename Parser>
inline constexpr auto many_to_state(Parser p) {
    return many_to_state([](auto &s) -> auto& {return s;}, p);
}
// Compile time recursive resolver for lifting of arbitrary number of parsers
template <bool Move, typename State, typename F, typename Parser, typename... Parsers>
static inline constexpr auto lift_or_rec(State &s, F f, Parser p, Parsers... ps) {
    if (auto result = p(s); has_result(result)) {
        // We can move the result when we have control over the supplied function
        if constexpr (Move) {
            return return_success(f(std::move(get_result(result))));
        } else {
            return return_success(f(get_result(result)));
        }
    } else if constexpr (sizeof...(ps) > 0) {
        return lift_or_rec<Move>(s, f, ps...);
    } else {
        // All parsers failed
        using result_type = std::decay_t<decltype(f(get_result(result)))>;
        return return_fail<result_type>();
    }
}

/**
 * Lift the result of a unary function to the parser monad after applying it to the first successful
 * parser's result.
 * The lifted function must provide an overload for every parser result type.
 */
template <typename F, typename Parser, typename... Parsers>
inline constexpr auto lift_or(F f, Parser p, Parsers... ps) {
    return parser([=](auto &s) {
        return lift_or_rec<false>(s, f, p, ps...);
    });
}

/**
 * Lift a type to the parser monad after applying the first successful parser's result to its constructor.
 * The constructor must provide an overload for every parser result type.
 */
template <typename Fun, typename Parser, typename... Parsers>
inline constexpr auto lift_or_state(Fun f, Parser p, Parsers... ps) {
    return parser([=](auto &s) {
        auto to_apply = [f, &s] (auto &val) {
             return f(s.user_state, val);
        };
        return lift_or_rec<false>(s, to_apply, p, ps...);
    });
}

/**
 * Lift a type to the parser monad after applying the first successful parser's result to its constructor.
 * The constructor must provide an overload for every parser result type.
 */
template <typename T, typename Parser, typename... Parsers>
inline constexpr auto lift_or_value(Parser p, Parsers... ps) {
    return parser([=](auto &s) {
        constexpr auto construct = [](auto&& arg) {return T(std::forward<decltype(arg)>(arg));};
        // We let the recursive function move the argument into the above function because the caller
        // will never see it anyway.
        return lift_or_rec<true>(s, construct, p, ps...);
    });
}

/**
 * Lift a type to the parser monad after applying the first successful parser's result to its constructor.
 * The constructor must provide an overload for every parser result type.
 * This version applies the constructor to a lazy argument
 */
template <typename T, typename Parser, typename... Parsers>
inline constexpr auto lift_or_value_from_lazy(Parser p, Parsers... ps) {
    return parser([=](auto &s) {
        constexpr auto construct = [](auto arg) {
            return T(arg());
        };
        return lift_or_rec<false>(s, construct, p, ps...);
    });
}

}

#endif // PARSER_COMBINATORS_H
