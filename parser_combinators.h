#ifndef PARSER_COMBINATORS_H
#define PARSER_COMBINATORS_H

#include <type_traits>
#include <unordered_map>
#include <vector>
#include "parser_core.h"
#include "monad.h"
#include "parse_algorithm.h"

namespace parse {

/**
 * Transform a parser to a parser that always succeeds.
 * Will return true as result if the parser succeeded, and false otherwise.
 * The result of the parse will be ignored.
 */
template <typename Parser>
inline constexpr auto succeed(Parser p) {
    return parser([=](auto &s) {
        if (apply(p, s)) {
            return s.return_success(true);
        } else {
            return s.return_success(false);
        }
    });
}

/**
 * Change the error to be returned upon a failed parse with the provided parser
 */
template <typename Parser, typename Error>
inline constexpr auto change_error(Parser p, Error error) {
    return parser([=](auto &s) {
        if (auto result = apply(p, s)) {
            return result;
        } else {
            using return_type = std::decay_t<decltype(*result)>;
            return s.template return_fail<return_type>(error);
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
        using result_type = std::decay_t<decltype(*(apply(p, s)))>;
        constexpr auto empty = [](auto &t) {
            if constexpr (std::is_integral<result_type>::value) {
                return t == 0;
            } else {
                return std::empty(t);
            }
        };
        if (auto result = apply(p, s); !result || !empty(*result)) {
            return result;
        } else {
            using return_type = std::decay_t<decltype(*result)>;
            return s.template return_fail<return_type>();
        }
    });
}

/**
 * Make a parser that doesn't consume its input on failure
 */
template <typename Parser>
inline constexpr auto try_parser(Parser p) {
    return parser([=](auto &s) {
        auto old_position = s.position;
        auto result = apply(p, s);
        if (!result) {
            s.position = old_position;
        }
        return result;
    });
}

/**
 * Make a parser non-consuming
 */
template <typename Parser>
inline constexpr auto no_consume(Parser p) {
    return parser([=](auto &s) {
        auto old_position = s.position;
        auto result = apply(p, s);
        s.position = old_position;
        return result;
    });
}

/**
 * Constrain a parser.
 * Takes in addition to a parser a predicate that takes the resulting type of the parser as argument.
 */
template <typename Parser, typename Predicate>
inline constexpr auto constrain(Parser p, Predicate pred) {
    return parser([=](auto &s) {
        if (auto result = apply(p, s); !result || pred(*result)) {
            return result;
        } else {
            return s.template return_fail<std::decay_t<decltype(*result)>>();
        }
    });
}

/**
 * Recursive helper for `get_parsed`
 */
template <typename State, typename Iterator, typename Parser, typename... Parsers>
static inline constexpr auto get_parsed_recursive(State &s, Iterator original_position, Parser p, Parsers... ps) {
    if (auto result = apply(p, s)) {
        if constexpr (sizeof...(Parsers) == 0) {
            return s.return_success(s.convert(original_position, s.position));
        } else {
            return get_parsed_recursive(s, original_position, ps...);
        }
    } else {
        return s.return_fail(result);
    }
}

/**
 * Make a parser that evaluates an arbitrary number of parsers, and returns the successfully parsed text upon
 * success.
 */
template <typename Parser, typename ... Parsers>
inline constexpr auto get_parsed(Parser p, Parsers ... ps) {
    return parser([=](auto &s) {
        return get_parsed_recursive(s, s.position, p, ps...);
    });
}

/**
 * Make a parser that evaluates two parsers, and returns the successfully parsed text upon success.
 */
template <typename P1, typename P2>
inline constexpr auto operator+(P1 p1, P2 p2) {
    return get_parsed(p1, p2);
}

/**
 * Combine two parsers so that the second will be tried before failing.
 * If the two parsers return different types the return value will instead be `true`.
 */
template <typename P1, typename P2>
inline constexpr auto operator||(P1 p1, P2 p2) {
    return parser([=](auto &s) {
        using R1 = decltype(*apply(p1, s));
        using R2 = decltype(*apply(p2, s));
        auto original_position = s.position;
        if constexpr (std::is_same_v<R1, R2>) {
            if (auto result1 = apply(p1, s)) {
                return result1;
            } else {
                s.position = original_position;
                return apply(p2, s);
            }
        } else {
            if (apply(p1, s)) {
                return s.return_success(true);
            } else {
                s.position = original_position;
                return apply(p2, s) ? s.return_success(true) : s.template return_fail<bool>();
            }
        }
    });
}

template <typename ... Parsers>
inline constexpr auto first(Parsers ... ps) {
    return (ps || ...);
}

/**
 * Modify the supplied user state.
 * Will use the returned value from the user supplied function as result,
 * or `true` if return type is `void`.
 */
template <typename Fun>
inline constexpr auto modify_state(Fun f) {
    return parser([=](auto &s) {
        using result_type = std::decay_t<decltype(f(s.user_state))>;
        if constexpr (std::is_void<result_type>::value) {
            f(s.user_state);
            return s.return_success(true);
        } else {
            return s.return_success(f(s.user_state));
        }
    });
}

/**
 * Set a value in the state accessed by `Accessor` from the result of the parser.
 * Note: Make sure that a reference is returned by `acc`.
 */
template <typename Parser, typename Accessor>
inline constexpr auto set_in_state(Parser p, Accessor acc) {
    return parser([=](auto &s) {
        auto result = apply(p, s);
        if (result) {
            acc(s.user_state) = *result;
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
        return apply(monad::lift(to_apply, ps...), s);
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
 * General helper for evaluating a parser multiple times with an optional separator.
 */
template <typename State, typename Fun, typename Parser, typename Sep = std::nullptr_t>
inline constexpr auto many_internal(State &s, Fun f, Parser p, Sep sep = nullptr) {
    size_t successful = 0;
    for (auto res = apply(p, s); res; res = apply(p, s)) {
        ++successful;
        f(*res);
        if constexpr (!std::is_null_pointer_v<Sep>) {
            if (!apply(sep, s)) break;
        }
    }
    return successful;
}

/**
 * General parser that saves results to collections.
 * The template argument `Container` should be default constructible, and ´inserter´
 * is a callable taking a `Container` as a reference as the first argument, and the
 * result of the parse as the second.
 */
template <typename Container, typename Inserter, typename Parser, typename ParserSep = std::nullptr_t>
inline constexpr auto many_general(Inserter inserter, Parser p, ParserSep sep = nullptr) {
    return parser([=](auto &s) {
        Container c;
        many_internal(s, [=](auto &&res) {inserter(c, std::forward<decltype(res)>(res));}, p, sep);
        return s.return_success(c);
    });
}

/**
 * Create a parser that applies a parser until it fails and returns the result in a vector.
 */
template <typename Parser, typename ParserSep = std::nullptr_t>
inline constexpr auto many_to_vector(Parser p, ParserSep sep = nullptr) {
    return parser([=](auto &s) {
        using result_type = std::decay_t<decltype(*apply(p, s))>;
        std::vector<result_type> r;
        many_internal(s, [&r](auto &&res) {r.emplace_back(std::forward<decltype(res)>(res));}, p, sep);
        return s.return_success(r);
    });
}

/**
 * Create a parser that applies a parser until it fails and returns the result in an `unordered_map`.
 * The parser must have a result type defining `first_type` and `second_type` (e.g. `std::pair`).
 */
template <typename Parser, typename ParserSep = std::nullptr_t>
inline constexpr auto many_to_unordered_map(Parser p, ParserSep sep = nullptr) {
    return parser([=](auto &s) {
        using result_type = std::decay_t<decltype(*apply(p, s))>;
        using key = typename result_type::first_type;
        using value = typename result_type::second_type;
        std::unordered_map<key, value> m;
        many_internal(s, [&](auto &&r) {
            m.insert(std::forward<decltype(r)>(r));
        }, p, sep);
        return s.return_success(m);
    });
}

/**
 * Create a parser that applies a parser until it fails, and for each successful parse
 * calls the provided callable `f` which should take the result of a successful parse with ´p´ as its
 * argument.
 * The parse result is the number of successful parses.
 */
template <typename Fun, typename Parser, typename ParserSep = std::nullptr_t>
inline constexpr auto many_f(Fun f, Parser p, ParserSep sep = nullptr) {
    return parser([=](auto &s) {
        return s.return_success(many_internal(s, [f](auto &&res){f(std::forward<decltype(res)>(res));}, p, sep));
    });
}

/**
 * Create a parser that applies a parser until it fails, and returns the parsed range as
 * returned by the provided conversion function.
 */
template <typename Parser, typename ParserSep = std::nullptr_t>
inline constexpr auto many(Parser p, ParserSep sep = nullptr) {
    return parser([=](auto &s) {
        auto start = s.position;
        many_internal(s, [](auto &&){}, p, sep);
        return s.return_success(s.convert(start, s.position));
    });
}

/**
 * Create a parser that applies a parser until it fails, and for each successful parse
 * calls the provided callable `f` which should take the user state by reference as its
 * first parameter, and the result of a successful parse as its second.
 * The parse result is the number of successful parses.
 */
template <typename Fun, typename Parser, typename ParserSep = std::nullptr_t>
inline constexpr auto many_state(Fun f, Parser p, ParserSep sep = nullptr) {
    return parser([=](auto &s) {
        return s.return_success(many_internal(s, [f, &s](auto &&res){f(s.user_state, std::forward<decltype(res)>(res));}, p, sep));
    });
}

// Compile time recursive resolver for lifting of arbitrary number of parsers
template <typename State, typename F, typename Parser, typename... Parsers>
static inline constexpr auto lift_or_rec(State &s, F f, Parser p, Parsers... ps) {
    if (auto result = apply(p, s)) {
        return s.return_success(f(std::move(*result)));
    } else if constexpr (sizeof...(ps) > 0) {
        return lift_or_rec(s, f, ps...);
    } else {
        // All parsers failed
        using result_type = std::decay_t<decltype(f(*result))>;
        if constexpr (State::error_handling) {
            return s.template return_fail<result_type>(result.error());
        } else {
            return s.template return_fail<result_type>();
        }
    }
}

/**
 * Lift the result of a unary functor to the parser monad after applying it to the first successful
 * parser's result.
 * The lifted functor must provide an overload for every parser result type.
 */
template <typename F, typename Parser, typename... Parsers>
inline constexpr auto lift_or(F f, Parser p, Parsers... ps) {
    return parser([=](auto &s) {
        return lift_or_rec(s, f, p, ps...);
    });
}

/**
 * Lift the result of a unary functor to the parser monad after applying it to the first successful
 * parser's result.
 * The lifted functor must provide an overload for every parser result type.
 * This is similar to `lift_or` but also passes along the user state to the provided ´Fun` as
 * its first argument. In addition, all `()` overloads in the functor must be marked `const`
 */
template <typename Fun, typename Parser, typename... Parsers>
inline constexpr auto lift_or_state(Fun f, Parser p, Parsers... ps) {
    return parser([=](auto &s) {
        auto to_apply = [f, &s] (auto &&val) {
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
    return lift_or([](auto&& arg) {return T(std::forward<decltype(arg)>(arg));}, p, ps...);
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

/**
 * Create a parser that parses the result of `p1` with `p2`.
 * Useful for parsing the content between brackets.
 * Currently only working if the return type from the provided conversion function
 * has the same iterator type as what is provided when starting a parse.
 */
template <typename Parser1, typename Parser2>
inline constexpr auto parse_result(Parser1 p1, Parser2 p2) {
    return parser([=](auto &s) {
        if (auto result = apply(p1, s)) {
            auto result_text = *result;
            using state_type = std::decay_t<decltype(s)>;
            state_type new_state(std::begin(result_text), std::end(result_text), s);
            auto new_result = apply(p2, new_state);
            return new_result;
        } else {
            return s.template return_fail<std::decay_t<decltype(*apply(p2, s))>>(*result);
        }
    });
}

/**
 * Parse all items until the supplied parser succeeds.
 * Use template parameter `Eat` to specify whether or not to consume the successful parse,
 * and `Include` to control whether to or not to include the successful parse in the result.
 * Note: For parsing until a certain sequence or item, use functions
 * `until_item` and `until_sequence` instead, as they are more efficient.
 */
template <bool Eat = true, bool Include = false, typename Parser>
inline constexpr auto until(Parser p) {
    return parser([=](auto &s) {
        // Must check if we are at the end here, otherwise we would proceed past s.end
        // if the parser fails (which is normal for this combinator).
        if (s.position == s.end) {
            return s.return_fail();
        }

        auto position_start = s.position;
        auto position_end = position_start;
        for (auto result = apply(p, s); !result; result = apply(p, s)) {
            s.advance(1);
            if (s.position == s.end) {
                s.position = position_start;
                return s.return_fail(result);
            }
            position_end = s.position;
        }

        auto end_pos = Include ? s.position : position_end;
        if constexpr (!Eat) {
            s.set_position(position_end);
        }

        return s.return_success(s.convert(position_start, end_pos));
    });
}

template <typename ReturnType, typename ErrorType = void, typename F>
constexpr auto recurse(F f) {
    return parser([f](auto &s) {
        constexpr auto return_it = [](auto &&r) {
            return parser([=](auto &) {
                return r;
            });
        };

        auto rec = [f, return_it](auto self, auto &s) -> decltype(return_it(std::declval<parse::result<ReturnType, ErrorType>>())) {
            auto r = parser([self](auto &s) {
                return apply(self(self, s), s);
            });
            return return_it(apply(f(r), s));
        };
        return apply(rec(rec, s), s);
    });
}


}

#endif // PARSER_COMBINATORS_H
