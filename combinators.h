#ifndef PARSER_COMBINATORS_H
#define PARSER_COMBINATORS_H

#include <type_traits>
#include <unordered_map>
#include <map>
#include <vector>
#include <memory>
#include "core.h"
#include "monad.h"
#include "combinators_internal.h"

namespace parse {

/**
 * Transform a parser to a parser that always succeeds.
 * Will return an `std::optional` with the result.
 */
template <typename Parser>
inline constexpr auto succeed(Parser p) {
    return parser([=](auto& s) {
        if (auto&& result = apply(p, s)) {
            return s.template return_success_emplace
                    <std::optional<std::decay_t<decltype(*result)>>>(*std::forward<decltype(result)>(result));
        } else {
            return s.template return_success_emplace<std::optional<std::decay_t<decltype(*result)>>>();
        }
    });
}

/**
 * Apply a parser `n` times and return the parsed result.
 */
template <typename Parser>
inline constexpr auto times(size_t n, Parser p) {
    return parser([=](auto& s) {
        return internal::times(s, n, p);
    });
}

/**
 * Apply a parser `n` times and return the parsed result.
 * Templated version.
 */
template <size_t N, typename Parser>
inline constexpr auto times(Parser p) {
    return parser([=](auto& s) {
        return internal::times(s, N, p);
    });
}

/**
 * Change the error to be returned upon a failed parse with the provided parser
 */
template <typename Parser, typename Error>
inline constexpr auto change_error(Parser p, Error error) {
    return parser([=](auto& s) {
        if (auto result = apply(p, s)) {
            return result;
        } else {
            using return_type = std::decay_t<decltype(*result)>;
            return s.template return_fail_error<return_type>(error);
        }
    });
}

/**
 * Make a parser that doesn't consume its input on failure
 */
template <typename Parser>
inline constexpr auto try_parser(Parser p) {
    return parser([=](auto& s) {
        auto old_position = s.position;
        auto result = apply(p, s);
        if (!result) {
            s.set_position(old_position);
        }
        return result;
    });
}

/**
 * Make a parser non-consuming
 */
template <typename Parser>
inline constexpr auto no_consume(Parser p) {
    return parser([=](auto& s) {
        auto old_position = s.position;
        auto result = apply(p, s);
        s.set_position(old_position);
        return result;
    });
}

/**
 * Constrain a parser.
 * Takes in addition to a parser a predicate that takes the resulting type of the parser as argument.
 */
template <typename Parser, typename Predicate>
inline constexpr auto constrain(Parser p, Predicate pred) {
    return parser([=](auto& s) {
        if (auto result = apply(p, s); !result || pred(*result)) {
            return result;
        } else {
            return s.template return_fail<std::decay_t<decltype(*result)>>();
        }
    });
}

/**
 * Transform a parser to a parser that fails on a successful, but empty result (as decided by std::empty
 * or == 0 if integral)
 */
template <typename Parser>
inline constexpr auto not_empty(Parser p) {
    return constrain(p, [](auto&& res) {
        if constexpr (std::is_integral_v<std::decay_t<decltype(res)>>) {
            return res != 0;
        } else {
            return !std::empty(res);
        }
    });
}

/**
 * Make a parser that evaluates an arbitrary number of parsers, and returns the successfully parsed text upon
 * success.
 */
template <typename Parser, typename ... Parsers>
inline constexpr auto get_parsed(Parser p, Parsers ... ps) {
    return parser([=](auto& s) {
        return internal::get_parsed_recursive(s, s.position, p, ps...);
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
 * If the two parsers return different types the return value will instead be `none`.
 */
template <typename P1, typename P2>
inline constexpr auto operator||(P1 p1, P2 p2) {
    return parser([=](auto& s) {
        using R1 = decltype(*apply(p1, s));
        using R2 = decltype(*apply(p2, s));
        auto original_position = s.position;
        if constexpr (std::is_same_v<R1, R2>) {
            if (auto result1 = apply(p1, s)) {
                return result1;
            } else {
                s.set_position(original_position);
                return apply(p2, s);
            }
        } else {
            if (apply(p1, s)) {
                return s.template return_success_emplace<none>();
            } else {
                s.set_position(original_position);
                auto result2 = apply(p2, s);
                return result2 ? s.template return_success_emplace<none>() :
                                      s.template return_fail_change_result<none>(result2);
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
 * or `none` if return type is `void`.
 */
template <typename Fun>
inline constexpr auto modify_state(Fun f) {
    return parser([=](auto& s) {
        using result_type = std::decay_t<decltype(f(s.user_state))>;
        if constexpr (std::is_void<result_type>::value) {
            f(s.user_state);
            return s.template return_success_emplace<none>();
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
    return parser([=](auto& s) {
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
    return parser([=](auto& s) {
        auto& state = s.user_state;
        auto to_apply = [f, &state] (auto&&... vals) {
            return f(state, std::forward<decltype(vals)>(vals)...);
        };
        return apply(lift(to_apply, ps...), s);
    });
}

/**
 * Emplace a value in the state with `emplace` with the results of a number
 * of parsers evaluated in sequence to the user supplied state accessed by `acc`.
 */
template <typename Accessor, typename... Parsers>
inline constexpr auto emplace_to_state(Accessor acc, Parsers... ps) {
    return apply_to_state([acc](auto& s, auto&&...args) {
        return acc(s).emplace(std::forward<decltype(args)>(args)...);
    }, ps...);
}

/**
 * Emplace a value in the state with `emplace` with the results of a number
 * of parsers evaluated in sequence to the user supplied state.
 */
template <typename... Parsers>
inline constexpr auto emplace_to_state_direct(Parsers... ps) {
    return emplace_to_state([](auto& s) -> auto& {return s;}, ps...);
}

/**
 * Emplace a value in the state with `emplace_back` with the results of a number
 * of parsers evaluated in sequence to the user supplied state accessed by `acc`.
 */
template <typename Accessor, typename... Parsers>
inline constexpr auto emplace_back_to_state(Accessor acc, Parsers... ps) {
    return apply_to_state([acc](auto& s, auto&&...args) {
        return acc(s).emplace_back(std::forward<decltype(args)>(args)...);
    }, ps...);
}

/**
 * Emplace a value in the state with `emplace_back` with the results of a number
 * of parsers evaluated in sequence to the user supplied state.
 */
template <typename... Parsers>
inline constexpr auto emplace_back_to_state_direct(Parsers... ps) {
    return emplace_back_to_state([](auto& s) -> auto& {return s;}, ps...);
}

/**
 * General parser that saves results to collections.
 * The template argument `Container` should be default constructible, and ´inserter´
 * is a callable taking a `Container` as a reference as the first argument, and the
 * result of the parse as the second.
 */
template <typename Container,
          typename Parser,
          typename Inserter,
          typename ParserSep = none,
          typename Break = none>
inline constexpr auto many_general(Parser p,
                                   Inserter inserter,
                                   ParserSep sep = {}) {
    return parser([=](auto& s) {
        Container c;
        internal::many(s, p, sep, [&c, inserter](auto&& res) {
            inserter(c, std::forward<decltype(res)>(res));
        });
        return s.return_success(std::move(c));
    });
}

/**
 * Create a parser that applies a parser until it fails and returns the result in an `std::vector`.
 */
template <typename Parser,
          typename ParserSep = none>
inline constexpr auto many_to_vector(Parser p,
                                     ParserSep sep = {}) {
    return parser([=](auto& s) {
        using result_type = std::decay_t<decltype(*apply(p, s))>;
        std::vector<result_type> r;
        internal::many(s, p, sep, [&r](auto&& res) {
            r.emplace_back(std::forward<decltype(res)>(res));
        });
        return s.return_success(std::move(r));
    });
}

/**
 * Create a parser that applies a parser until it fails and returns the result in an `std::array`.
 * The result is an `std::pair` where the first element is the resulting array, and the second
 * the first index after the last inserted result.
 */
template <size_t size,
          typename Parser,
          typename ParserSep = none,
          typename Break = none>
inline constexpr auto many_to_array(Parser p,
                                    ParserSep sep = {}) {
    return parser([=](auto& s) {
        using result_type = std::decay_t<decltype(*apply(p, s))>;
        std::array<result_type, size> arr{};
        size_t i = 0;
        internal::many(s, p, sep, [&arr, &i](auto&& res) {
            arr[i++] = res;
        });
        return s.template return_success_emplace<std::pair<std::array<result_type, size>, size_t>>(std::move(arr), i);
    });
}

/**
 * Create a parser that applies a parser until it fails and returns the result in an
 * `std::unordered_map`.
 * Use the first template argument to specify unordered or not. Default is unordered.
 * Key and value are retrieved from the result using std::tuple_element.
 */
template <bool Unordered = true,
          typename Parser,
          typename ParserSep = none>
inline constexpr auto many_to_map(Parser p,
                                  ParserSep sep = {}) {
    return parser([=](auto& s) {
        using result_type = std::decay_t<decltype(*apply(p, s))>;
        using key = std::tuple_element_t<0, result_type>;
        using value = std::tuple_element_t<1, result_type>;
        using map_type = std::conditional_t<Unordered, std::unordered_map<key, value>, std::map<key, value>>;
        map_type m;
        internal::many(s, p, sep, [&m](auto&& r) {
            m.emplace(std::get<0>(std::forward<decltype(r)>(r)),
                      std::get<1>(std::forward<decltype(r)>(r)));
        });
        return s.return_success(std::move(m));
    });
}

/**
 * Create a parser that applies a parser until it fails, and for each successful parse
 * calls the provided callable `f` which should take the result of a successful parse with ´p´ as its
 * argument.
 * The parse result is the parsed range as returned by the provided conversion function.
 */
template <typename Parser,
          typename Fun,
          typename ParserSep = none>
inline constexpr auto many_f(Parser p,
                             Fun f,
                             ParserSep sep = {}) {
    return parser([=](auto& s) {
        return internal::many(s, p, sep, [f](auto&& r) {
            f(std::forward<decltype(r)>(r));
        });
    });
}

/**
 * Create a parser that applies a parser until it fails, and returns the parsed range as
 * returned by the provided conversion function.
 */
template <typename Parser,
          typename ParserSep = none>
inline constexpr auto many(Parser p,
                           ParserSep sep = {}) {
    return parser([=](auto& s) {
        return internal::many(s, p, sep);
    });
}

/**
 * Create a parser that applies a parser until it fails, and for each successful parse
 * calls the provided callable `f` which should take the user state by reference as its
 * first parameter, and the result of a successful parse as its second.
 * The parse result is the parsed range as returned by the provided conversion function.
 */
template <typename Parser,
          typename Fun,
          typename ParserSep = none>
inline constexpr auto many_state(Parser p,
                                 Fun f,
                                 ParserSep sep = {}) {
    return parser([=](auto& s) {
        return internal::many(s, p, sep, [f, &s](auto&& res) {
            f(s.user_state, std::forward<decltype(res)>(res));
        });
    });
}

/**
 * Fold a series of successful parser results with binary functor `f` and initial value `i`.
 * Use template parameter `FailOnNoSuccess` to decide if to fail when the parser succeeds 0 times.
 * Use template parameter `Mutate` to mutate your accumulator value rather than returning a new one.
 * If not using `Mutate` the functor cannot take its accumulator parameter by l-value reference.
 */
template <bool FailOnNoSuccess = false,
          bool Mutate = false,
          typename Parser,
          typename Init,
          typename Fun,
          typename ParserSep = none>
inline constexpr auto fold(Parser p,
                           Init&& i,
                           Fun f,
                           ParserSep sep = {}) {
    return parser([p, i = std::forward<Init>(i), f, sep](auto& s) mutable {
        [[maybe_unused]] auto result = internal::many<FailOnNoSuccess>(s, p, sep, [f, &i](auto&& a) {
            if constexpr (Mutate)  f(i, std::forward<decltype(a)>(a));
            else i = f(std::move(i), std::forward<decltype(a)>(a));
        });
        if constexpr (FailOnNoSuccess) {
            if (!result) {
                return s.template return_fail_change_result<Init>(result);
            }
        }
        return s.return_success(std::move(i));
    });
}

/**
 * Lift the result of a unary functor to the parser monad after applying it to the first successful
 * parser's result.
 * The lifted functor must provide an overload for every parser result type.
 */
template <typename F, typename Parser, typename... Parsers>
inline constexpr auto lift_or(F f, Parser p, Parsers... ps) {
    return parser([=](auto& s) {
        return internal::lift_or_rec(s, f, p, ps...);
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
    return parser([=](auto& s) {
        auto to_apply = [f, &s] (auto&& val) {
            return f(s.user_state, std::forward<decltype(val)>(val));
        };
        return internal::lift_or_rec(s, to_apply, p, ps...);
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

/**
 * Create a parser that parses the result of `p1` with `p2`.
 * Useful for parsing the content between brackets.
 * Currently only working if the return type from the provided conversion function
 * has the same iterator type as what is provided when starting a parse.
 */
template <typename Parser1, typename Parser2>
inline constexpr auto parse_result(Parser1 p1, Parser2 p2) {
    return parser([=](auto& s) {
        if (auto&& result = apply(p1, s)) {
            auto result_text = std::move(*result);
            using state_type = std::decay_t<decltype(s)>;
            state_type new_state(std::begin(result_text), std::end(result_text), s);
            auto new_result = apply(p2, new_state);
            return new_result;
        } else {
            return s.template return_fail_change_result<std::decay_t<decltype(*apply(p2, s))>>(result);
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
    return parser([=](auto& s) {
        auto position_start = s.position;
        auto position_end = position_start;
        for (auto result = apply(p, s); !result; result = apply(p, s)) {
            if (s.empty()) {
                s.set_position(position_start);
                return s.return_fail_result_default(result);
            }
            s.advance(1);
            position_end = s.position;
        }

        auto end_pos = Include ? s.position : position_end;
        if constexpr (!Eat) {
            s.set_position(position_end);
        }

        return s.return_success(s.convert(position_start, end_pos));
    });
}

/**
 * Create a recursive parser.
 * Provide a callable taking an `auto` as its parameter, and returning
 * a parser. The passed parser is functionally identical to the returned parser,
 * and can be used recursively within that.
 * You must provide the `ReturnType` of the parsers as the first template argument
 * (to make the compiler happy).
 */
template <typename ReturnType, typename F>
constexpr auto recursive(F f) {
    return parser([f](auto& s) {
        auto rec = [f, &s](auto self)
                -> parse::result<ReturnType, typename std::decay_t<decltype(s)>::error_type> {
            auto p = parser([self](auto&) { // The actual parser sent to the caller.
                return self(self);
            });
            return apply(f(p), s);
        };
        return rec(rec);
    });
}

}

#endif // PARSER_COMBINATORS_H
