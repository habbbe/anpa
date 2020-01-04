#ifndef PARSIMON_PARSERS_H
#define PARSIMON_PARSERS_H

#include <charconv>
#include "parsimon/internal/algorithm.h"
#include "parsimon/core.h"
#include "parsimon/types.h"
#include "parsimon/combinators.h"
#include "parsimon/internal/parsers_internal.h"
#include "parsimon/internal/pow10.h"

namespace parsimon {

/**
 * Parser that always succeeds
 */
inline constexpr auto success() {
    return parser([](auto& s) {
        return s.template return_success_emplace<none>();
    });
}

/**
 * Parser with result type `T` that always fails.
 *
 * @tparam T the result type of the parse. Default is `none`.
 */
template <typename T = none>
inline constexpr auto fail() {
    return parser([](auto& s) {
        return s.template return_fail<T>();
    });
}

/**
 * Parser for the empty sequence
 */
inline constexpr auto empty() {
    return parser([](auto& s) {
        return s.empty() ? s.template return_success_emplace<none>() :
                           s.template return_fail<none>();
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
 * Parser for a single specific item.
 *
 * @param i the item to parse.
 */
template <typename ItemType>
inline constexpr auto item(ItemType&& i) {
    return parser([i = std::forward<ItemType>(i)](auto& s) {
        return internal::item(s, [&i](const auto &c) {return c == i;});
    });
}

/**
 * Parser for a single specific item. Templated version.
 * This might be faster than the non-templated version due to less copying.
 *
 * @tparam i the item to parse.
 */
template <auto i>
inline constexpr auto item() {
    return parser([](auto& s) {
        return internal::item(s, [](const auto &c) {return c == i;});
    });
}

/**
 * Parser for a single item not equal to `i`

 * @param i the item to parse.
 */
template <typename ItemType>
inline constexpr auto not_item(ItemType&& i) {
    return parser([i = std::forward<ItemType>(i)](auto& s) {
        return internal::item(s, [&i](const auto &c) {return c != i;});
    });
}

/**
 * Parser for a single item not equal to `i`. Templated version.
 * This might be faster than the non-templated version due to less copying.
 *
 */
template <auto i>
inline constexpr auto not_item() {
    return parser([](auto& s) {
        return internal::item(s, [](const auto &c) {return c != i;});
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
 * Parser for the sequence described by `[begin, end)`
 */
template <typename BeginIt, typename EndIt>
inline constexpr auto seq(BeginIt&& begin, EndIt&& end) {
    return parser([b = std::forward<BeginIt>(begin), e = std::forward<EndIt>(end)](auto& s) {
        return internal::seq(s, std::distance(b, e),
                             [=](auto i) {return algorithm::equal(b, e, i);});
    });
}

/**
 * Parser for the sequence described by the template parameters.
 * This might give better performance than the non-templated version due to less copying.
 */
template <auto v, auto... vs>
inline constexpr auto seq() {
    return parser([](auto& s) {
        return internal::seq(s, sizeof...(vs) + 1,
                             [](auto i){return algorithm::equal<v, vs...>(i);});
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
template <typename BeginIt, typename EndIt>
inline constexpr auto any_of(BeginIt&& begin, EndIt&& end) {
    return item_if([b = std::forward<BeginIt>(begin), e = std::forward<EndIt>(end)](const auto& c) {
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
template <auto v, auto... vs>
inline constexpr auto any_of() {
    return item_if([](const auto& c) {
        return algorithm::contains<v, vs...>(c);
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
template <size_t n>
inline constexpr auto consume() {
    return parser([](auto& s) {
        return internal::consume(s, n);
    });
}

/**
 * Parser for consuming all items up until a certain item `c`.
 *
 * @tparam Eat decides whether or not to consume the successful parse
 * @tparam Include decides whether or not to include the succesful parse in
 *                 the result.
 *
 */
template <bool Eat = true, bool Include = false, typename ItemType>
inline constexpr auto until_item(ItemType&& c) {
    return parser([c = std::forward<ItemType>(c)](auto& s) {
        return internal::until_item<Eat, Include>(s, c);
    });
}

/**
 * Templated version of `until_item`.
 * This might give better performance than the non-templated version due to less copying.
 *
 * @tparam Eat decides whether or not to consume the successful parse
 * @tparam Include decides whether or not to include the succesful parse in
 *                 the result.
 */
template <auto item, bool Eat = true, bool Include = false>
inline constexpr auto until_item() {
    return parser([](auto& s) {
        return internal::until_item<Eat, Include>(s, item);
    });
}

/**
 * Parser for consuming all items up until a certain sequence described by `[begin, end)`.
 *
 * This may be faster than using `until(sequence())`.
 *
 * @tparam Eat decides whether or not to consume the successful parse
 * @tparam Include decides whether or not to include the succesful parse in
 *                 the result.
 */
template <bool Eat = true, bool Include = false, typename Iterator>
inline constexpr auto until_seq(Iterator begin, Iterator end) {
    return parser([=](auto& s) {
        return internal::until_seq<Eat, Include>(s,
                    [=](auto b, auto e) {return algorithm::search(b, e, begin, end);});
    });
}

/**
 * Parser for consuming all items up until the given null-terminated string literal `seq`.
 *
 * This may be faster than using `until(sequence())`.
 *
 * @tparam Eat decides whether or not to consume the successful parse
 * @tparam Include decides whether or not to include the succesful parse in
 *                 the result.
 */
template <bool Eat = true,
          bool Include = false,
          typename ItemType, size_t N,
          typename = types::enable_if_string_literal_type<ItemType>>
inline constexpr auto until_seq(const ItemType (&seq)[N]) {
    return until_seq<Eat, Include>(std::begin(seq), std::end(seq) - 1);
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
 * This parser will never fail.
 *
 * The parse result is the parsed range as returned by the provided conversion function.
 */
template <typename Predicate>
inline constexpr auto while_predicate(Predicate predicate) {
    return parser([=](auto& s) {
        auto start_pos = s.position;
        auto result = algorithm::find_if_not(start_pos, s.end, predicate);
        s.set_position(result);
        return s.return_success(s.convert(start_pos, result));
    });
}

/**
 * Parser that consumes all items in the set described by `[start, end)`
 *
 * The parse result is the parsed range as returned by the provided conversion function.
 */
template <typename Iterator>
inline constexpr auto while_in(Iterator start, Iterator end) {
    return while_predicate([=](const auto& val){return algorithm::contains(start, end, val);});
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
template <auto v, auto... vs>
inline constexpr auto while_in() {
    return while_predicate([](const auto& val){
        return algorithm::contains<v, vs...>(val);
    });
}

/**
 * Parser that consumes all items between the two provided null-terminated sequences.
 *
 * @tparam Nested decides wheter or not to support nested matchings.
 * @tparam Include decides whether or not to include the succesful parse in
 *                 the result.
 *
 */
template <bool Nested = false, bool Include = false, typename ItemType, size_t NStart, size_t NEnd>
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

    return internal::between_general<NStart-1, NEnd-1, Nested, Include>(start, end, compar_start, compare_end);
}

/**
 * Parser that consumes all items between the provided items `start` and `end`.
 *
 * @tparam Nested decides wheter or not to support nested matchings.
 * @tparam Include decides whether or not to include the succesful parse in
 *                 the result.
 */
template <bool Nested = false, bool Include = false, typename ItemType>
inline constexpr auto between_items(const ItemType start, const ItemType end) {
    constexpr auto compare_single = [](const auto iterator, auto, const auto& toCompare) {
        return *iterator == toCompare;
    };
    return internal::between_general<1, 1, Nested, Include>(start, end, compare_single, compare_single);
}

/**
 * Create a custom parser.
 *
 * @param custom_parser a functor with the following signature:
 *                        (Iterator position, Iterator End) -> std::pair<Iterator, std::optional<Result>>
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
 *                        (Iterator position, Iterator End, State& s) -> std::pair<Iterator, std::optional<Result>>
 *                      where the first element in the pair is the new iterator position, and the second
 *                      the result, where and empty optional signals a failed parse.
 *
 */
template <typename Parser>
inline constexpr auto custom_with_state(Parser custom_parser) {
    return parser([=](auto& s) {
        return internal::custom(s, custom_parser(s.position, s.end, s.user_state));
    });
}

// CONVENIENCE PARSERS

/**
 * Parse a number. Template parameter indicates the type to be parsed. Uses std::from_chars.
 * This parser only works when using `const *char` as iterator.
 * Consider using `integer` or `floating` instead, as they are constexpr, and performance
 * should be comparable.
 */
template <typename Number>
inline constexpr auto number() {
    return parser([](auto& s) {
        auto [start, end] = [](auto& state) {
            if constexpr (std::is_pointer_v<decltype(state.position)>) {
                return std::pair(state.position, state.end);
            } else {
                return std::pair(&*state.position, &*state.end);
            }
        }(s);

        Number result;
        auto [ptr, ec] = std::from_chars(start, end, result);
        if (ec == std::errc()) {
            s.set_position(ptr);
            return s.return_success(result);
        } else {
            return s.template return_fail<Number>();
        }
    });
}

/**
 * Parser for an integer.
 *
 * @tparam Integral specifies which type of integer to parse.
 */
template <typename Integral = int, bool IncludeDoubleDivisor = false>
inline constexpr auto integer() {

    auto res_parser = [](bool neg) {
        auto p = fold<true, true>(item_if([](const auto& c) {return c >= '0' && c <= '9';}),
                                  std::pair<Integral, unsigned>(0, 1), [](auto& r, const auto& c) {
            if constexpr (IncludeDoubleDivisor) {
                r.second *= 10;
            }
            r.first = r.first * 10 + c - '0';
        });
        return lift([=](auto&& res) {
            if (neg) res.first = -res.first;
            if constexpr (IncludeDoubleDivisor) {
                return std::forward<decltype(res)>(res);
            } else {
                return std::forward<decltype(res)>(res).first;
            }
        }, p);
    };
    if constexpr (std::is_signed_v<Integral>) {
        return succeed(item<'-'>()) >>= [=](auto&& neg) {
            return res_parser(neg.has_value());
        };
    } else {
        return res_parser(false);
    }
}

/**
 * Parser for a floating number.
 * @tparam FloatType specifies which type of floating number to parse.
 * @tparam AllowScientific enable/disable support for scientific notation.
 */
template <typename FloatType = double, bool AllowScientific = true>
inline constexpr auto floating() {
    auto floating_part = integer() >>= [](auto&& n) {
        auto dec = item<'.'>() >> integer<unsigned int, true>();
        return lift([=](auto&& p) {
            // ((0 <= n) - (n < 0)) returns -1 for n < 0 otherwise 1
            return n + ((0 <= n) - (n < 0)) * int(p.first) / FloatType(p.second);
        }, dec) || mreturn_emplace<FloatType>(std::forward<decltype(n)>(n));
    };

    if constexpr (AllowScientific) {
        return floating_part >>= [](auto&& f) {
            auto exp = any_of<'e', 'E'>() >> integer();
            return lift([=](auto&& e) { return f * internal::pow_table<FloatType>::pow(e); }, exp)
                    || mreturn(std::forward<decltype(f)>(f));
        };
    } else {
        return floating_part;
    }
}

/**
 * Parser for whitespace
 */
inline constexpr auto whitespace() {
    return while_predicate([](const auto& c) {
        return c == ' ' || (c >= '\t' && c <= '\r');
    });
}

}

#endif // PARSIMON_PARSERS_H
