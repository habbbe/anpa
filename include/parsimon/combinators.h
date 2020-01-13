#ifndef PARSIMON_COMBINATORS_H
#define PARSIMON_COMBINATORS_H

#include <type_traits>
#include <unordered_map>
#include <map>
#include <vector>
#include <memory>
#include "parsimon/core.h"
#include "parsimon/monad.h"
#include "parsimon/internal/combinators_internal.h"
#include "parsimon/options.h"

namespace parsimon {

/**
 * Transform a parser to a parser that always succeeds.
 * Will return `true` if the parse was successful, else 'false`.
 *
 * @tparam Options available options:
 * 				     `options::optional`: return the parse result as an optional instead of `true`/`false`.
 */
template <options Options = options::none, typename Parser>
inline constexpr auto succeed(Parser p) {
    return parser([=](auto& s) {
        if constexpr (has_options(Options, options::optional)) {
            using optional_type = std::optional<std::decay_t<decltype(*apply(p, s))>>;
            if (auto&& result = apply(p, s)) {
                return s.template return_success_emplace<optional_type>(*std::forward<decltype(result)>(result));
            } else {
                return s.template return_success_emplace<optional_type>();
            }
        } else {
            if (apply(p, s)) {
                return s.return_success(true);
            } else {
                return s.return_success(false);
            }
        }

    });
}

/**
 * Apply a parser `n` times and return the parsed result.
 *
 * @param n the number of times to apply the parser
 */
template <typename Size, typename Parser>
inline constexpr auto times(Size&& n, Parser p) {
    return parser([n = std::forward<Size>(n), p](auto& s) {
        return internal::times(s, n, p);
    });
}

/**
 * Apply a parser `n` times and return the parsed result.
 * Templated version.
 *
 * @tparam N the number of times to apply the parser
 */
template <size_t N, typename Parser>
inline constexpr auto times(Parser p) {
    return parser([=](auto& s) {
        return internal::times(s, N, p);
    });
}

/**
 * Change the error to be returned upon a failed parse with the provided parser
 *
 * @param error the new error to return upon failure
 */
template <typename Error, typename Parser>
inline constexpr auto change_error(Error&& error, Parser p) {
    return parser([error = std::forward<Error>(error), p](auto& s) {
        if (auto result = apply(p, s)) {
            return result;
        } else {
            return s.template return_fail_error<decltype(*result)>(error);
        }
    });
}

/**
 * Make a parser non-consuming.
 *
 * @tparam Options available options:
 * 				     `options::failure_only`: make the parser non-consuming on failure only
 *                                            (this is the same as `try_parser`)
 *
 */
template <options Options = options::none, typename Parser>
inline constexpr auto no_consume(Parser p) {
    return parser([=](auto& s) {
        auto old_position = s.position;
        auto result = apply(p, s);
        if (!has_options(Options, options::failure_only) || !result) {
            s.set_position(old_position);
        }
        return result;
    });
}

/**
 * Make a parser that doesn't consume its input on failure
 *
 * This is the same as `no_consume<options::failure_only>`
 */
template <typename Parser>
inline constexpr auto try_parser(Parser p) {
    return no_consume<options::failure_only>(p);
}

/**
 * Constrain a parser.
 *
 * @param pred a predicate with the signature:
 *               `BOOL_CONVERTIBLE(const auto& parse_result)`
 *
 */
template <typename Predicate, typename Parser>
inline constexpr auto constrain(Predicate pred, Parser p) {
    return parser([=](auto& s) {
        if (auto result = apply(p, s); !result || pred(*result)) {
            return result;
        } else {
            return s.template return_fail<std::decay_t<decltype(*result)>>();
        }
    });
}

/**
 * Transform a parser to a parser that fails on a successful, but empty result
 * (as decided by `std::empty`) or `== 0` if integral (as decided by `std::is_integral`).
 */
template <typename Parser>
inline constexpr auto not_empty(Parser p) {
    return constrain([](auto&& res) {
        if constexpr (std::is_integral_v<std::decay_t<decltype(res)>>) {
            return res != 0;
        } else {
            return !std::empty(res);
        }
    }, p);
}

/**
 * Make a parser that evaluates an arbitrary number of parsers, and returns the successfully parsed
 * range as returned by the provided conversion function.
 */
template <typename... Parsers>
inline constexpr auto get_parsed(Parsers... ps) {
    types::assert_parsers_not_empty<Parsers...>();
    return parser([=](auto& s) {
        return internal::get_parsed_recursive(s, s.position, ps...);
    });
}

/**
 * Make a parser that evaluates two parsers, and returns the successfully parsed range
 * as returned by the provided conversion function.
 */
template <typename P1, typename P2>
inline constexpr auto operator+(parser<P1> p1, parser<P2> p2) {
    return get_parsed(p1, p2);
}

/**
 * Combine two parsers so that the second will be tried before failing.
 * If the two parsers return different types the return value will instead be `empty_result`.
 */
template <typename P1, typename P2>
inline constexpr auto operator||(parser<P1> p1, parser<P2> p2) {
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
                return s.template return_success_emplace<empty_result>();
            } else {
                s.set_position(original_position);
                auto result2 = apply(p2, s);
                return result2 ? s.template return_success_emplace<empty_result>() :
                                 s.template return_fail_change_result<empty_result>(result2);
            }
        }
    });
}

/**
 * Variadic version of `||` for easier typing.
 */
template <typename... Parsers>
inline constexpr auto first(Parsers... ps) {
    types::assert_parsers_not_empty<Parsers...>();
    return (ps || ...);
}

/**
 * Use the user state to construct a new parser.
 *
 * This combinator can be used to create dynamic parsers that
 * depend on some state.
 *
 * @param f a functor with the signature:
 *            `ParserType(auto& user_state)`
 */
template <typename Fn>
inline constexpr auto with_state(Fn f) {
    return parser([=](auto& s) {
        return apply(f(s.user_state), s);
    });
}

/**
 * Modify the user state.
 * Will use the returned value from the user supplied function as result,
 * or `empty_result` if return type is `void`.
 *
 * @param f a functor with the signature:
 *            `ResultType(auto& user_state)`
 */
template <typename Fn>
inline constexpr auto modify_state(Fn f) {
    return parser([=](auto& s) {
        using result_type = decltype(f(s.user_state));
        if constexpr (std::is_void_v<result_type>) {
            f(s.user_state);
            return s.template return_success_emplace<empty_result>();
        } else {
            return s.return_success(f(s.user_state));
        }
    });
}

/**
 * Apply a functor to the state after evaluating a number of parsers in sequence.
 *
 * The result of the parse is the return value of `f`, or `empty_result` if `f` returns `void`.
 *
 * @param f a functor with the signature:
 *            `ResultTypeOrVoid(auto& state, auto&&... results)`
 */
template <typename Fn, typename... Parsers>
inline constexpr auto apply_to_state(Fn f, Parsers...ps) {
    types::assert_parsers_not_empty<Parsers...>();
    return parser([=](auto& s) {
        types::assert_functor_application_modify<decltype(s), Fn, decltype((s.user_state)), Parsers...>();

        auto to_apply = [f, &state = s.user_state] (auto&&... vals) {
            if constexpr (std::is_void_v<decltype(f(state, std::forward<decltype(vals)>(vals)...))>) {
                f(state, std::forward<decltype(vals)>(vals)...);
            } else {
                return f(state, std::forward<decltype(vals)>(vals)...);
            }
        };
        return apply(lift(to_apply, ps...), s);
    });
}

/**
 * Create a parser that applies a parser until it fails and adds the results to a `std::vector`.
 * Use template argument `reserve` to reserve storage before parsing.
 *
 * @tparam Options available options:
 * 				     `options::no_trailing_separator`: disallow a trailing separator
 *
 * @tparam Reserve use to reserve space before commencing parsing
 *
 * @param separator an optional separator. Use `{}` to ignore.
 *
 * @param inserter an optional functor that should have the signature:
 *                   `void(auto& vec, auto&& result)`.
 *                 Use to change how/if results are inserted. Use `{}` to use default
 *                 insertion (`push_back`).
 */
template <options Options = options::none,
          size_t Reserve = 0,
          typename Parser,
          typename ParserSep = no_arg,
          typename Inserter = no_arg
          >
inline constexpr auto many_to_vector(Parser p,
                                     ParserSep separator = {},
                                     Inserter inserter = {}) {
    return parser([=](auto& s) {
        using result_type = std::decay_t<decltype(*apply(p, s))>;
        auto ins = internal::default_arg(inserter, [](auto& v, auto&& rs) {
            v.push_back(std::forward<decltype(rs)>(rs));
        });

        using vector_type = std::vector<result_type>;

        types::assert_functor_application_modify<decltype(s), decltype(ins), vector_type, Parser>();

        auto init = []() {
            if constexpr (Reserve > 0) {
                return [](auto& v) {v.reserve(Reserve);};
            } else {
                return no_arg();
            }
        }();

        return internal::fold_internal<Options>(s, init, ins, vector_type{}, separator, p);
    });
}

/**
 * Shorthand for ::many_to_vector
 * @sa ::many_to_vector
 */
template <typename P>
inline constexpr auto operator*(parser<P> p) {
    return many_to_vector(p);
}

/**
 * Shorthand for `not_empty(many_to_vector)`
 */
template <typename P>
inline constexpr auto operator+(parser<P> p) {
    return not_empty(many_to_vector(p));
}

/**
 * Create a parser that applies a parser until it fails and adds the results to an `std::array`.
 *
 * The parse result is an `std::pair` where the first element is the resulting array, and the second
 * the first index after the last inserted result.
 *
 * @tparam size the size of the array
 *
 * @tparam Options available options:
 * 				     `options::no_trailing_separator`: disallow a trailing separator
 *
 * @param separator an optional separator. Use `{}` to ignore.
 */
template <size_t Size,
          options Options = options::none,
          typename Parser,
          typename ParserSep = no_arg>
inline constexpr auto many_to_array(Parser p,
                                    ParserSep separator = {}) {
    return parser([=](auto& s) {
        using result_type = std::decay_t<decltype(*apply(p, s))>;
        std::array<result_type, Size> arr{};
        size_t i = 0;
        auto result = internal::many_internal<Options>(s, [&arr, &i](auto&& res) {
            arr[i++] = std::forward<decltype(res)>(res);
        }, separator, p);

        using parse_result = std::pair<std::array<result_type, Size>, size_t>;

        if constexpr (types::has_arg<ParserSep> && has_options(Options, options::no_trailing_separator)) {
            if (!result) {
                return s.template return_fail_change_result<parse_result>(result);
            }
        }
        return s.template return_success_emplace<parse_result>(std::move(arr), i);
    });
}

/**
 * Create a parser that applies two parsers until failure and returns the results in an
 * `std::unordered_map` (or `std::map` if `options::ordered` is used as an option).
 * By default uses the result of the first parser as key and the result of the
 * second parser as value. Use template arguments `Key` and `Value` to override.
 *
 * Note: If `Key` or `Value` is overridden, and the parse results are not convertible
 * to those types, `inserter` will need to be specified.
 *
 * @tparam Options available options:
 * 				     `options::ordered`: use `std::map` instead of `std::unordered_map`
 * 				     `options::no_trailing_separator`: disallow a trailing separator
 *
 * @tparam Key use to override key type
 * @tparam Value use to override value type
 *
 * @param key_parser parser for key
 * @param value_parser parser for value
 * @param separator an optional separator. Use `{}` to ignore.
 * @param inserter an optional functor that should have the signature:
 *                   `void(auto& map, auto&& key, auto&& value)`.
 *                 Use to change how/if results are inserted. Use `{}` to use default
 *                 insertion (`emplace`).
 */
template <options Options = options::none,
          typename Key = no_arg,
          typename Value = no_arg,
          typename KeyParser,
          typename ValueParser,
          typename ParserSep = no_arg,
          typename Inserter = no_arg>
inline constexpr auto many_to_map(KeyParser key_parser,
                                  ValueParser value_parser,
                                  ParserSep separator = {},
                                  Inserter inserter = {}) {
    return parser([=](auto& s) {
        using key = std::conditional_t<types::has_arg<Key>, Key, std::decay_t<decltype(*apply(key_parser, s))>>;
        using value = std::conditional_t<types::has_arg<Value>, Value, std::decay_t<decltype(*apply(value_parser, s))>>;
        using map_type = std::conditional_t<has_options(Options, options::ordered), std::map<key, value>, std::unordered_map<key, value>>;
        auto ins = internal::default_arg(inserter, [](auto& map, auto&&... rs) {
            map.emplace(std::forward<decltype(rs)>(rs)...);
        });

        types::assert_functor_application_modify<decltype(s), decltype(ins), map_type, KeyParser, ValueParser>();

        return internal::fold_internal<Options>(s, {}, ins, map_type{}, separator, key_parser, value_parser);
    });
}

/**
 * Create a parser that applies a number of parsers until it fails, and for each successful parse
 * calls the provided functor `f` with the results.
 *
 * The parse result is the parsed range as returned by the provided conversion function.
 *
 * @tparam Options available options:
 * 				     `options::no_trailing_separator`: disallow a trailing separator
 *
 * @param f a functor to be called for all successful parses. It should have the signature:
 *            `void(auto&&... results)`
 *
 * @param separator an optional separator. Use `{}` to ignore.
 */
template <options Options = options::none,
          typename Fn = no_arg,
          typename ParserSep = no_arg,
          typename... Parsers>
inline constexpr auto many_f(Fn f,
                             ParserSep separator,
                             Parsers... ps) {
    types::assert_parsers_not_empty<Parsers...>();
    return parser([=](auto& s) {
        types::assert_functor_application<decltype(s), Fn, Parsers...>();
        return internal::many_internal<Options>(s, f, separator, ps...);
    });
}

/**
 * Create a parser that applies a parser until it fails, and returns the parsed range as
 * returned by the provided conversion function.
 *
 * Note that this function isn't variadic like `many_f`. This is due to the parse results
 * being ignored. To evaluate multiple parsers, just combine them with `>>`.
 *
 * @tparam Options available options:
 * 				     `options::no_trailing_separator`: disallow a trailing separator
 *
 * @param separator an optional separator. Use `{}` to ignore.
 */
template <options Options = options::none,
          typename Parser,
          typename ParserSep = no_arg>
inline constexpr auto many(Parser p, ParserSep separator = {}) {
    return many_f<Options>({}, separator, p);
}

/**
 * Create a parser that applies a number of parsers until it fails, and for each successful parse
 * calls the provided functor `f` with the user state and the results.
 *
 * The parse result is the parsed range as returned by the provided conversion function.
 *
 * @tparam Options available options:
 * 				     `options::no_trailing_separator`: disallow a trailing separator
 *
 * @param f a functor to be called for all successful parses. It should have the signature:
 *            `void(auto& state, auto&&... results)`
 *
 * @param separator an optional separator. Use `{}` to ignore.
 */
template <options Options = options::none,
          typename Fn,
          typename ParserSep = no_arg,
          typename... Parsers>
inline constexpr auto many_state(Fn f,
                                 ParserSep separator,
                                 Parsers... ps) {
    types::assert_parsers_not_empty<Parsers...>();
    return parser([=](auto& s) {
        types::assert_functor_application_modify<decltype(s), Fn, decltype((s.user_state)), Parsers...>();
        return internal::many_internal<Options>(s, [f, &s](auto&& res) {
            f(s.user_state, std::forward<decltype(res)>(res));
        }, separator, ps...);
    });
}

/**
 * Fold a series of successful parser results with binary functor `f` and initial
 * accumulator value `acc`.
 *
 * @tparam Options available options:
 * 				     `options::no_trailing_separator`: disallow a trailing separator
 * 				     `options::fail_on_no_parse`: fail if the parser succeeds 0 times
 * 				     `options::replace`: replace the accumulator instead of mutating it
 *
 * @param acc the initial accumulator value
 *
 * @param f a functor to be called for all successful parses. It should have the signature:
 *            `void(auto& accumulator, auto&& result)`
 *          or if non-mutating:
 *            `(auto&& accumulator, auto&& result) -> decltype(accumulator)`
 *
 * @param separator an optional separator. Use `{}` to ignore.
 */
template <options Options = options::none,
          typename Fn,
          typename Acc,
          typename ParserSep = no_arg,
          typename... Parsers
          >
inline constexpr auto fold(Fn f,
                           Acc&& acc,
                           ParserSep separator,
                           Parsers... ps) {
    return parser([f, acc = std::forward<Acc>(acc), separator, ps...](auto& s) {
        return internal::fold_internal<Options>(s, {}, f, acc, separator, ps...);
    });
}

/**
 * Fold a series of successful parser results with binary functor `f` and initial
 * accumulator value as `InitType{}`.
 *
 * This might be faster than `fold` for default-constructible types due to less copying.
 *
 * @tparam InitType the accumulator type. Must be default constructible.
 *
 * @tparam Options available options:
 * 				     `options::no_trailing_separator`: disallow trailing separator
 * 				     `options::fail_on_no_parse`: fail if the parser succeeds 0 times
 * 				     `options::replace`: replace the accumulator instead of mutating it
 *
 * @param f a functor to be called for all successful parses. It should have the signature:
 *            `void(auto& accumulator, auto&& result)`
 *          or if non-mutating:
 *            `(auto&& accumulator, auto&& result) -> decltype(accumulator)`
 *
 * @param separator an optional separator. Use `{}` to ignore.
 */
template <typename InitType,
          options Options = options::none,
          typename Init = no_arg,
          typename Fn,
          typename ParserSep = no_arg,
          typename... Parsers>
inline constexpr auto fold_direct(
                           Init init,
                           Fn f,
                           ParserSep separator,
                           Parsers... ps) {
    return parser([init, f, separator, ps...](auto& s) {
        return internal::fold_internal<Options>(s, init, f, InitType{}, separator, ps...);
    });
}

/**
 * Lift the result of a unary functor to the parser monad after applying it
 * to the first successful parser's result.
 * The lifted functor must provide an overload for every parser result type.
 *
 * @param f a functor with the signature:
 *            `ResultType(auto&& result)`
 *          it must be overloaded for every possible parser result type.
 */
template <typename Fn, typename... Parsers>
inline constexpr auto lift_or(Fn f, Parsers... ps) {
    types::assert_parsers_not_empty<Parsers...>();
    return parser([=](auto& s) {
        (types::assert_functor_application<decltype(s), Fn, Parsers>(), ...);
        return internal::lift_or_rec(s, s.position, f, ps...);
    });
}

/**
 * Lift the result of a unary functor to the parser monad after applying it
 * to the first successful parser's result.
 * The lifted functor must provide an overload for every parser result type.
 *
 * This is similar to `lift_or` but also passes along the user state to `f` as
 * its first argument.`
 *
 * @param f a functor with the signature:
 *            `ResultType(auto& state, auto&& result)`
 *          it must be overloaded for every possible parser result type.
 */
template <typename Fn, typename... Parsers>
inline constexpr auto lift_or_state(Fn f, Parsers... ps) {
    types::assert_parsers_not_empty<Parsers...>();
    return parser([=](auto& s) {
        (types::assert_functor_application_modify<decltype(s), Fn, decltype((s.user_state)), Parsers>(), ...);
        auto to_apply = [f, &s] (auto&& val) {
            return f(s.user_state, std::forward<decltype(val)>(val));
        };
        return internal::lift_or_rec(s, s.position, to_apply, ps...);
    });
}

/**
 * Lift a type to the parser monad after applying the first successful parser's
 * result to its constructor. The constructor must provide an overload for every
 * parser result type.
 *
 * @tparam T the type to be constructed from the parse result. The constructor
 *           for `T` must be overloaded for all possible parse results.
 */
template <typename T, typename... Parsers>
inline constexpr auto lift_or_value(Parsers... ps) {
    types::assert_parsers_not_empty<Parsers...>();
    return lift_or([](auto&& arg) {return T(std::forward<decltype(arg)>(arg));}, ps...);
}

/**
 * Create a parser that parses the result of `p1` with `p2`.
 * `std::begin` and `std::end` will be used with the result of `p1` for the
 * parse with `p2`, that is, the iterator type can be changed for the second parse.
 *
 * The default conversion function will return a `range` result which can be used with
 * this combinator.
 *
 * @tparam NewSettings use this template parameter if you want to change the parse settings
 *                     for the parse with `p2`.
 *
 * @param p1 the parser to apply first
 * @param p2 the parser to apply to the result of `p1`
 */
template <typename NewSettings = no_arg, typename Parser1, typename Parser2>
inline constexpr auto parse_result(Parser1 p1, Parser2 p2) {
    return parser([=](auto& s) {
        if (auto&& result = apply(p1, s)) {
            auto result_text = std::move(*result);
            using state_type = std::decay_t<decltype(s)>;

            auto new_state = [&](){
                constexpr auto new_settings = []() {
                    if constexpr (types::has_arg<NewSettings>)
                        return NewSettings();
                    else
                        return typename state_type::settings();
                }();
                if constexpr (state_type::has_user_state) {
                return parser_state(std::begin(result_text), std::end(result_text), s.user_state, new_settings);
            } else {
                return parser_state_simple(std::begin(result_text), std::end(result_text), new_settings);
            }

            }();

            return apply(p2, new_state);
        } else {
            return s.template return_fail_change_result<decltype(*apply(p2, s))>(result);
        }
    });
}

/**
 * Parse all items until the supplied parser succeeds.
 *
 * This parser does not consume anything if the parser never succeeds.
 *
 * Note: For parsing until a certain sequence or item, use functions
 * `until_item` and `until_seq` instead, as they are more efficient.
 *
 * @tparam Options available options:
 * 				     `options::dont_eat`: do not consume the successful parse
 * 				     `options::include`: include the parsed item in the result
 */
template <options Options = options::none, typename Parser>
inline constexpr auto until(Parser p) {
    return parser([=](auto& s) {
        auto position_start = s.position;
        auto position_end = position_start;

        for (auto result = apply(p, s); !result; result = apply(p, s)) {
            if (s.at_end()) {
                s.set_position(position_start);
                return s.return_fail_result_default(result);
            }
            s.advance(1);
            position_end = s.position;
        }

        auto end_pos = has_options(Options, options::include) ? s.position : position_end;
        if constexpr (has_options(Options, options::dont_eat)) {
            s.set_position(position_end);
        }

        return s.return_success(s.convert(position_start, end_pos));
    });
}

/**
 * (Description inspired by Parsec's `chainl1`)
 * Chain one or more `p` separated by `op`.
 * `op` is a parser that returns a binary functor taking as arguments the
 * return type of `p`.
 *
 * This parser will return the repeated left associative application of
 * the functor returned by `op` applied to the results of `p`.
 *
 * This parser can be used to eliminate left recursion, for example in expression
 * grammars. See `test/tests_calc.cpp` for an example on how to use it.
 *
 * @param p a parser that returns some value `V`
 * @param op a parser that returns a binary functor with signatre:
 *             `V(V, V)`
 */
template <typename Parser, typename OpParser>
constexpr auto chain(Parser p, OpParser op) {
    return parser([=](auto& s) {
        auto result1 = apply(p, s);
        if (!result1) return result1;

        auto r = *std::move(result1);
        for (;;) {
            auto opRes = apply(op, s);
            if (!opRes) return s.return_success(r);

            auto result2 = apply(p, s);
            if (!result2) return s.return_success(r);

            r = (*opRes)(r, *result2);
        }
    });
}

/**
 * Create a recursive parser.
 * Provide a lambda taking an `auto` as its parameter, and returning
 * a parser. The passed parser is functionally identical to the returned parser,
 * and can be used recursively within that.
 * You must provide the `ReturnType` of the parsers as the first template argument
 * (to make the compiler happy).
 *
 * Beware of left recursion, or you will segfault.
 *
 * Example of a parser that parses an integer arbitrarily nested in braces:
 * @code
 * constexpr auto rec_parser = recursive<int>([](auto p) {
 *      return integer() || (item<'{'>() >> p << item<'}'>());
 * });
 * @endcode
 *
 * this will parse a string on the format "{{{{{{{{123}}}}}}}}"
 *
 * @tparam ReturnType the result type for the parser.
 *
 * @param f a lambda with the format:
 *            `[](auto p) {...} -> RecursiveParser`
 *
 */
template <typename ReturnType, typename Fn>
constexpr auto recursive(Fn f) {
    return parser([f](auto& s) {
        auto rec = [f, &s](auto self)
                -> result<ReturnType, typename std::decay_t<decltype(s)>::error_type> {
            auto p = parser([self](auto&) { // The actual parser sent to the caller.
                return self(self);
            });
            return apply(f(p), s);
        };
        return rec(rec);
    });
}

}

#endif // PARSIMON_COMBINATORS_H
