#ifndef PARSIMON_PARSERS_H
#define PARSIMON_PARSERS_H

#include "parsimon/internal/algorithm.h"
#include "parsimon/core.h"
#include "parsimon/options.h"
#include "parsimon/types.h"
#include "parsimon/combinators.h"
#include "parsimon/internal/parsers_internal.h"
#include "parsimon/internal/pow10.h"

namespace parsimon {

/**
 * Parser that always succeeds.
 */
inline constexpr auto success() {
    return parser([](auto& s) {
        return s.template return_success_emplace<empty_result>();
    });
}

/**
 * Parser with result type `T` that always fails.
 *
 * @tparam T the result type of the parse. Default is `empty_result`.
 */
template <typename T = empty_result>
inline constexpr auto fail() {
    return parser([](auto& s) {
        return s.template return_fail<T>();
    });
}

/**
 * Create a parser that will succeed only when `condition` is `true`.
 * This parser does not consume any input.
 *
 * Result of the parse is `empty_result`.
 *
 * This primitive can be used to create parsers that depend on the result of a previous parse.
 *
 * Example:
 * @code
 * constexpr auto zero_odd_even = integer() >>= [](auto i) {
 *    return cond(i == 0)     >> zero_parser ||
 *           cond(i % 2 == 0) >> even_parser ||
 *           odd_parser
 * };
 * @endcode
 *
 * @param condition the condition.
 */
inline constexpr auto cond(bool condition) {
    return parser([=](auto& s) {
        return condition ? s.template return_success_emplace<empty_result>() : s.template return_fail<empty_result>();
    });
}

/**
 * Parser for the empty sequence.
 */
inline constexpr auto empty() {
    return parser([](auto& s) {
        return s.at_end() ? s.return_success(s.convert(s.position)) :
                            s.return_fail();
    });
}

/**
 * Parser for any item
 */
inline constexpr auto any_item() {
    return parser([](auto& s) {
        return internal::item(s, [](const auto&) {return true;});
    });
}

/**
 * Parser for a single item equal to `item`, compared with `==`.
 *
 * @tparam Options available options:
 * 				     `options::return_arg`: return the argument instead of the
 *                                          parsed item upon success
 *
 * @param item the item to parse.
 */
template <options Options = options::none, typename ItemType>
inline constexpr auto item(ItemType&& item) {
    return parser([i = std::forward<ItemType>(item)](auto& s) {
        return internal::item<Options>(s, [](const auto &c, const auto& i) {return c == i;}, i);
    });
}

/**
 * Parser for a single item equal to `Item`, compared with `==`.
 *
 * Templated version.
 * This might be faster than the non-templated version due to less copying.
 *
 * @tparam Item the item to parse.
 *
 * @tparam Options available options:
 * 				     `options::return_arg`: return the argument instead of the
 *                                 parsed item upon success
 */
template <auto Item, options Options = options::none>
inline constexpr auto item() {
    return parser([](auto& s) {
        return internal::item<Options>(s, [](const auto &c, const auto& i) {return c == i;}, Item);
    });
}

/**
 * Parser for a single item not equal to `item`, compared with `!=`.

 * @param item the item to parse.
 */
template <typename ItemType>
inline constexpr auto not_item(ItemType&& item) {
    return parser([i = std::forward<ItemType>(item)](auto& s) {
        return internal::item(s, [](const auto &c, const auto& i) {return c != i;}, i);
    });
}

/**
 * Parser for a single item not equal to `Item`, compared with `!=`.
 *
 * Templated version.
 * This might be faster than the non-templated version due to less copying.
 *
 * @tparam Item the item to parse.
 */
template <auto Item>
inline constexpr auto not_item() {
    return parser([](auto& s) {
        return internal::item(s, [](const auto& c, const auto& i) {return c != i;}, Item);
    });
}

/**
 * Parser for a single item matching the provided predicate
 *
 * @param pred a predicate with the signature:
 *               `bool(const auto& item)`
 */
template <typename Pred>
inline constexpr auto item_if(Pred pred) {
    return parser([=](auto& s) {
        return internal::item(s, pred);
    });
}

/**
 * Parser for a single item not matching the provided predicate
 *
 * @param pred a predicate with the signature:
 *               `bool(const auto& item)`
 */
template <typename Pred>
inline constexpr auto item_if_not(Pred pred) {
    return parser([=](auto& s) {
        return internal::item(s, [=](const auto& p) {return !pred(p);});
    });
}

/**
 * Parser for the sequence described by `[begin, end)`
 */
template <typename InputIt>
inline constexpr auto seq(InputIt begin, InputIt end) {
    return parser([b = std::move(begin), e = std::move(end)](auto& s) {
        return internal::seq(s, std::distance(b, e),
                             [=](auto i) {return algorithm::equal(b, e, i);});
    });
}

/**
 * Parser for the sequence described by the template parameters.
 * This might give better performance than the non-templated version due to less copying.
 */
template <auto V, auto... Vs>
inline constexpr auto seq() {
    return parser([](auto& s) {
        return internal::seq(s, sizeof...(Vs) + 1,
                             [](auto i){return algorithm::equal<V, Vs...>(i);});
    });
}

/**
 * Parser for a sequence described by a null-terminated string literal
 */
template <typename ItemType, size_t N, typename = types::enable_if_string_literal_type<ItemType>>
inline constexpr auto seq(const ItemType (&items)[N]) {
    return seq(std::begin(items), std::end(items) - 1);
}

/**
 * Parser for any item contained in the set described by `[begin, end)`
 */
template <typename InputIt>
inline constexpr auto any_of(InputIt begin, InputIt end) {
    return item_if([b = std::move(begin), e = std::move(end)](const auto& c) {
        return algorithm::contains(b, e, c);
    });
}

/**
 * Parser for any item contained in the set described by the provided null-terminated string literal
 */
template <typename ItemType, size_t N, typename = types::enable_if_string_literal_type<ItemType>>
inline constexpr auto any_of(const ItemType (&set)[N]) {
    return any_of(std::begin(set), std::end(set) - 1);
}

/**
 * Parser for any item contained in the set described by the provided template arguments
 */
template <auto V, auto... Vs>
inline constexpr auto any_of() {
    return item_if([](const auto& c) {
        return algorithm::contains<V, Vs...>(c);
    });
}

/**
 * Parser for consuming `n` items
 */
inline constexpr auto consume(size_t n) {
    return parser([=](auto& s) {
        return internal::consume(s, n);
    });
}

/**
 * Parser for consuming `n` items. Templated version
 * This might give better performance than the non-templated version due to less copying.
 */
template <size_t N>
inline constexpr auto consume() {
    return parser([](auto& s) {
        return internal::consume(s, N);
    });
}

/**
 * Parser for consuming all items up until a certain item `c`.
 *
 *
 * @tparam Options available options:
 * 				     `options::dont_eat`: do not consume the successful parse
 * 				     `options::include`: include the parsed sequence in the result
 */
template <options Options = options::none, typename ItemType>
inline constexpr auto until_item(ItemType&& c) {
    return parser([c = std::forward<ItemType>(c)](auto& s) {
        return internal::until_item<Options>(s, c);
    });
}

/**
 * Templated version of `until_item`.
 * This might give better performance than the non-templated version due to less copying.
 *
 * @tparam Options available options:
 * 				     `options::dont_eat`: do not consume the successful parse
 * 				     `options::include`: include the parsed sequence in the result
 */
template <auto Item, options Options = options::none>
inline constexpr auto until_item() {
    return parser([](auto& s) {
        return internal::until_item<Options>(s, Item);
    });
}

/**
 * Parser for consuming all items up until a certain sequence described by `[begin, end)`.
 *
 * This may be faster than using `until(sequence())`.
 *
 * @tparam Options available options:
 * 				     `options::dont_eat`: do not consume the successful parse
 * 				     `options::include`: include the parsed sequence in the result
 */
template <options Options = options::none, typename InputIt>
inline constexpr auto until_seq(InputIt begin, InputIt end) {
    return parser([=](auto& s) {
        return internal::until_seq<Options>(s,
                    [=](auto b, auto e) {return algorithm::search(b, e, begin, end);});
    });
}

/**
 * Parser for consuming all items up until the given null-terminated string literal `seq`.
 *
 * This may be faster than using `until(sequence())`.
 *
 * @tparam Options available options:
 * 				     `options::dont_eat`: do not consume the successful parse
 * 				     `options::include`: include the parsed sequence in the result
 */
template <options Options = options::none,
          typename ItemType, size_t N,
          typename = types::enable_if_string_literal_type<ItemType>>
inline constexpr auto until_seq(const ItemType (&seq)[N]) {
    return until_seq<Options>(std::begin(seq), std::end(seq) - 1);
}

/**
 * Parser for the rest of the sequence
 *
 * The parse result is the parsed range as returned by the provided conversion function.
 */
inline constexpr auto rest() {
    return parser([](auto& s) {
        auto start_pos = s.position;
        s.set_position(s.end);
        return s.return_success(s.convert(start_pos, s.position));
    });
}

/**
 * Parser that consumes all items that matches the provided `predicate`.
 *
 * The parse result is the parsed range as returned by the provided conversion function.
 *
 * @tparam Options available options:
 * 				     `options::fail_on_no_parse`: fail if nothing is parsed
 * 				     `options::negate`: negate the predicate
 */
template <options Options = options::none, typename Predicate>
inline constexpr auto while_if(Predicate predicate) {
    return internal::while_if<Options>(predicate);
}

/**
 * Parser that consumes all items that does not match the provided `predicate`.
 *
 * The parse result is the parsed range as returned by the provided conversion function.
 *
 *
 * @tparam Options available options:
 * 				     `options::fail_on_no_parse`: fail if nothing is parsed
 */
template <options Options = options::none, typename Predicate>
inline constexpr auto while_if_not(Predicate predicate) {
    return while_if_not<Options | options::negate>(predicate);
}

/**
 * Parser that consumes all items in the set described by `[start, end)`
 *
 * The parse result is the parsed range as returned by the provided conversion function.
 */
template <typename InputIt>
inline constexpr auto while_in(InputIt start, InputIt end) {
    return while_if([=](const auto& val){return algorithm::contains(start, end, val);});
}

/**
 * Parser that consumes all items contained in the given null-terminated string literal.
 *
 * The parse result is the parsed range as returned by the provided conversion function.
 */
template <typename ItemType, size_t N, typename = types::enable_if_string_literal_type<ItemType>>
inline constexpr auto while_in(const ItemType (&items)[N]) {
    return while_in(std::begin(items), std::end(items) - 1);
}

/**
 * Parser that consumes all items contained in set described by the template parameters.
 * This might be faster than the non-templated version due to less copying.
 *
 * The parse result is the parsed range as returned by the provided conversion function.
 */
template <auto V, auto... Vs>
inline constexpr auto while_in() {
    return while_if([](const auto& val){return algorithm::contains<V, Vs...>(val);});
}

/**
 * Parser that consumes all items between the two provided null-terminated sequences.
 *
 * @tparam Options available options:
 * 				     `options::nested`: enables nested matchings
 * 				     `options::include`: include the parsed sequences in the result
 */
template <options Options = options::none, typename ItemType, size_t NStart, size_t NEnd>
inline constexpr auto between_sequences(const ItemType (&start)[NStart], const ItemType (&end)[NEnd]) {

    // Use faster comparison when the sequence is only one item long
    [[maybe_unused]] constexpr auto compare_single = [](auto begin, auto, auto toCompare) {
        return *begin == *toCompare;
    };
    [[maybe_unused]] constexpr auto compare_seq = [](auto begin, auto end, auto toCompare) {
        return algorithm::equal(begin, end, toCompare);
    };

    constexpr auto compar_start = [=]() {
        if constexpr (NStart - 1 == 1) return compare_single; else return compare_seq;
    }();
    constexpr auto compare_end = [=]() {
        if constexpr (NEnd - 1 == 1) return compare_single; else return compare_seq;
    }();

    return internal::between_general<NStart-1, NEnd-1, Options>(start, end, compar_start, compare_end);
}

/**
 * Parser that consumes all items between the provided items `start` and `end`.
 *
 * @tparam Options available options:
 * 				     `options::nested`: enables nested matchings
 * 				     `options::include`: include the parsed items in the result
 */
template <options Options = options::none, typename ItemType>
inline constexpr auto between_items(const ItemType start, const ItemType end) {
    constexpr auto compare_single = [](const auto iterator, auto, const auto& toCompare) {
        return *iterator == toCompare;
    };
    return internal::between_general<1, 1, Options>(start, end, compare_single, compare_single);
}

/**
 * Create a custom parser.
 *
 * @param custom_parser a functor with the following signature:
 *                        (InputIt position, InputIt End) -> std::pair<InputIt, std::optional<Result>>
 *                      where the first element in the pair is the new iterator position, and the second
 *                      the result, where and empty optional signals a failed parse.
 */
template <typename Parser>
inline constexpr auto custom(Parser custom_parser) {
    return parser([=](auto& s) {
        return internal::custom(s, custom_parser(s.position, s.end));
    });
}

/**
 * Create a custom parser with user state.
 *
 * @param custom_parser a functor with the following signature:
 *                        (InputIt position, InputIt End, State& s) -> std::pair<InputIt, std::optional<Result>>
 *                      where the first element in the pair is the new iterator position, and the second
 *                      the result, where and empty optional signals a failed parse.
 */
template <typename Parser>
inline constexpr auto custom_with_state(Parser custom_parser) {
    return parser([=](auto& s) {
        return internal::custom(s, custom_parser(s.position, s.end, s.user_state));
    });
}

// CONVENIENCE PARSERS

/**
 * Parser for an integer.
 *
 * @tparam Integral specifies which type of integer to parse.
 *
 * @tparam Options available options:
 * 				     `options::no_negative`: do not parse negative values. This has no effect
 *                                           if `Integral` is unsigned.
 * 				     `options::leading_plus`: allow a leading `+` sign.
 * 				     `options::no_leading_zero`: do not allow a leading zero (e,g. "01").
 *
 */
template <typename Integral = int, options Options = options::none, bool IncludeDoubleDivisor = false>
inline constexpr auto integer() {

    auto res_parser = [](bool neg) {

        auto is_digit = [](const auto&c) {return c >= '0' && c <= '9';};
        using pair_result = std::pair<Integral, unsigned>;
        auto int_parser = fold<options::fail_on_no_parse>([](auto& r, const auto& c) {
            if constexpr (IncludeDoubleDivisor) {
                r.second *= 10;
            }
            r.first = r.first * 10 + c - '0';
        }, pair_result(0, 1), {}, item_if(is_digit));
        auto p = [int_parser, is_digit](){
            if constexpr (has_options(Options, options::no_leading_zero)) {
                auto zero_result = lift([](){return pair_result();});
                return try_parser(item<'0'>() >> no_consume(flip(item_if(is_digit))) >> zero_result
                                      | int_parser);
            } else {
                return int_parser;
            }
        };
        return lift([=](auto&& res) {
            if (neg) res.first *= -1;
            if constexpr (IncludeDoubleDivisor) {
                return std::forward<decltype(res)>(res);
            } else {
                return std::forward<decltype(res)>(res).first;
            }
        }, p);
    };

    using leading_plus = std::bool_constant<has_options(Options, options::leading_plus)>;
    using leading_minus = std::bool_constant<std::is_signed_v<Integral> && !has_options(Options, options::no_negative)>;

    if constexpr (!leading_plus() && !leading_minus()) {
        return res_parser(false);
    } else {
        // We need try_parser here to not consume a token if input is "-" or "+"
        return try_parser([](){
            constexpr auto dash = item<'-'>();
            constexpr auto minus = dash >> mreturn<true>();
            constexpr auto plus  = succeed(item<'+'>()) >> mreturn<false>();
            if constexpr (leading_plus() && leading_minus()) {
                return minus || plus;
            } else if constexpr (!leading_plus() && leading_minus()) {
                return succeed(dash);
            } else {
                return plus;
            }
        }() >>= res_parser);
    }
}

/**
 * Parser for a floating number.
 *
 * @tparam FloatType specifies which type of floating number to parse.
 *
 * @tparam Options available options:
 * 				     `options::no_negative`: do not parse negative values.
 * 				     `options::leading_plus`: allow a leading `+` sign.
 * 				     `options::no_leading_zero`: do not allow a leading zero (e,g. "01.5").
 * 				     `options::no_scientific`: disable support for scientific notation
 * 				     `options::decimal_comma`: use decimal comma instead of period
 */

template <typename FloatType = double, options Options = options::none>
inline constexpr auto floating() {
    auto floating_part = integer<int, Options>() >>= [](auto&& n) {
        constexpr auto decimal_sign = [](){
            if constexpr (has_options(Options, options::decimal_comma)) return ',';
            else return '.';
        }();
        auto dec = item<decimal_sign>() >> integer<unsigned int, options::none, true>();
        return lift([=](auto&& p) {
            // ((0 <= n) - (n < 0)) returns -1 for n < 0 otherwise 1
            return n + ((0 <= n) - (n < 0)) * int(p.first) / FloatType(p.second);
        }, dec) || mreturn_emplace<FloatType>(std::forward<decltype(n)>(n));
    };

    if constexpr (has_options(Options, options::no_scientific)) {
        return floating_part;
    } else {
        return floating_part >>= [](auto&& f) {
            auto exp = any_of<'e', 'E'>() >> integer<int, options::leading_plus>();
            return lift([=](auto&& e) { return f * internal::pow_table<FloatType>::pow(e); }, exp)
                    || mreturn(std::forward<decltype(f)>(f));
        };
    }
}

/**
 * Parser that trims whitespace (if any)
 *
 * @tparam Options available options:
 * 				     `options::fail_on_no_parse`: fail if nothing is parsed
 * 				     `options::negate`: trim non-whitespace instead
 */
template <options Options = options::none>
inline constexpr auto trim() {
    return while_if<Options>([](const auto& c) {
        return c == ' ' || (c >= '\t' && c <= '\r');
    });
}

/**
 * Parser for a sequence of whitespaces
 */
inline constexpr auto whitespaces() {
    return trim<options::fail_on_no_parse>();
}

/**
 * Parser for a sequence of non-whitespace items
 */
inline constexpr auto not_whitespaces() {
    return trim<options::negate | options::fail_on_no_parse>();
}

}

#endif // PARSIMON_PARSERS_H
