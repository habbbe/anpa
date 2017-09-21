#ifndef PARSER_COMBINATORS_H
#define PARSER_COMBINATORS_H

#include "parser_core.h"

namespace parse {

/**
 * Transform a parser to a parser that always succeeds.
 * Will return true as result if the parser succeeded, and false otherwise
 */
template <typename Parser>
inline constexpr auto succeed(Parser p) {
    return parser([=](auto &s) {
        auto result = p(s);
        if (result) {
            return return_success(true);
        }
        return return_success(false);
    });
}

/**
 * Transform a parser to a parser that fails on a successful, but empty result (as decided by std::empty)
 */
template <typename Parser>
inline constexpr auto not_empty(Parser p) {
    return parser([=](auto &s) {
        if (auto result = p(s); result && !std::empty(*result)) {
            return return_success(*result);
        }
        using return_type = std::decay_t<decltype(*p(s))>;
        return std::optional<return_type>{};
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
        if (auto res = p(s); res) {
            return res;
        } else {
            using return_type = std::decay_t<decltype(*p(s))>;
            // Move back the old string
            s.rest = std::move(str_copy);
            return std::optional<return_type>{};
        }
    });
}

/**
 * Make a parser non consuming
 */
template <typename Parser>
inline constexpr auto no_consume(Parser p) {
    return parser([=](auto &s) {
        auto str_copy = s.rest;
        auto res = p(s);
        s.rest = std::move(str_copy);
        return res;
    });
}

/**
 * Combine two parsers so that the second will be tried before failing.
 * If the two parsers return different types the return value will instead be `true`.
 */
template <typename Parser1, typename Parser2>
inline constexpr auto operator||(Parser1 p1, Parser2 p2) {
    using R1 = decltype(*p1({}));
    using R2 = decltype(*p2({}));
    return parser([=](auto &s) {
        if constexpr (std::is_same_v<R1, R2>) {
            if (auto res1 = p1(s); res1) {
                return res1;
            } else if (auto res2 = p2(s); res2) {
                return res2;
            } else {
                using return_type = decltype(p1(s));
                return return_type{};
            }
        } else {
            if (auto res1 = p1(s); res1) {
                return return_success(true);
            } else if (auto res2 = p2(s); res2) {
                return return_success(true);
            } else {
                return return_fail_type<bool>();
            }
        }
    });
}

/**
 * Apply a provided function to the state after evaluating a number of parsers in sequence.
 * The function provided shall have the state as its first argument (by reference) and
 * then a number of arguments that matches the number of parsers (or variadic arguments)
 */
template <typename Fun, typename... Parsers>
inline constexpr auto apply_to_state(Fun f, Parsers&&...ps) {
    return parser([=](auto &s) {
        auto to_apply = [f, &s] (auto&&...vals) {
            return f(s.user_state, std::forward<decltype(vals)>(vals)...);
        };
        return monad::lift(to_apply, ps...)(s);
    });
}

/**
 * Create a parser that applies a parser until it fails or until the result doesn't satisfy
 * the user provided criteria.
 */
template <typename Container, typename Parser, typename Predicate>
inline constexpr auto many_if(Parser p, Predicate pred) {
    return parser([=](auto &s) {
        Container c;
        auto result = p(s);
        while (result && pred(*result)) {
            c.push_back(*result);
            result = p(s);
        }
        return return_success(std::move(c));
    });
}

/**
 * Create a parser that applies a parser until it fails
 */
template <typename Container, typename Parser>
inline constexpr auto many_value(Parser p) {
    return many_if<Container>(p, [](auto &){return true;});
}

/**
 * Create a parser that applies a parser until it fails or until the result doesn't satisfy
 * the user provided criteria and stores the result in a container in the user provided state
 * which is accessed by the function acc.
 * The number of added elements is then returned.
 * Note that this parser always succeeds, so you need to check the resulting number whether a
 * parse succeeded or not.
 */
template <typename Parser, typename Predicate, typename Accessor>
inline constexpr auto many_if_to_state(Parser p, Predicate pred, Accessor acc) {
    return parser([=](auto &s) {
        auto result = p(s);
        std::decay_t<decltype(acc(s.user_state).size())> number_of_results = 0;
        auto &c = acc(s.user_state);
        while (result && pred(*result)) {
            c.emplace_back(std::move(*result));
            result = p(s);
            ++number_of_results;
        }
        return return_success(number_of_results);
    });
}

/**
 * Create a parser that applies a parser until it fails or until the result doesn't satisfy
 * the user provided criteria and stores the result in the user provided state.
 * Note that the user provided state needs to be a container.
 */
template <typename Parser, typename Predicate>
inline constexpr auto many_if_to_state(Parser p, Predicate pred) {
    return many_if_to_state(p, pred, [](auto &s) {return s;});
}

/**
 * Create a parser that applies a parser until it fails and stores the result in the user
 * provided state which is accessed by the function acc.
 */
template <typename Parser, typename Accessor>
inline constexpr auto many_to_state(Parser p, Accessor acc) {
    return many_if_to_state(p, [](auto &) {return true;}, acc);
}

/**
 * Create a parser that applies a parser until it fails and stores the result in the user
 * provided state.
 * Note that the user provided state needs to be a container.
 */
template <typename Parser>
inline constexpr auto many_to_state(Parser p) {
    return many_to_state(p, [](auto &s) {return s;});
}

// Compile time recursive resolver for lifting of arbitrary number of parsers
template <typename State, typename F, typename Parser, typename... Parsers>
static inline constexpr auto lift_or_rec(State &s, F f, Parser p, Parsers... ps) {
    if (auto res = p(s); res) {
        return return_success(f(*res));
    } else if constexpr (sizeof...(ps) > 0) {
        return lift_or_rec(s, f, ps...);
    } else {
        // All parsers failed
        using result_type = std::decay_t<decltype(f(*p(s)))>;
        return return_fail_type<result_type>();
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
        return lift_or_rec(s, f, p, ps...);
    });
}

/**
 * Lift a type to the parser monad after applying the first successful parser's result to its constructor.
 * The constructor must provide an overload for every parser result type.
 */
template <typename Fun, typename Parser, typename... Parsers>
inline constexpr auto lift_or_state(Fun f, Parser p, Parsers... ps) {
    return parser([=](auto &s) {
        auto to_apply = [f, &s] (auto&& val) {
             return f(s.user_state, std::forward<decltype(val)>(val));
        };
        return lift_or_rec(s, to_apply, p, ps...);
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
        return lift_or_rec(s, construct, p, ps...);
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
        return lift_or_rec(s, construct, p, ps...);
    });
}

}

#endif // PARSER_COMBINATORS_H
