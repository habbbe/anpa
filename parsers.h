#ifndef PARSER_PARSERS_H
#define PARSER_PARSERS_H

#include <charconv>
#include "algorithm.h"
#include "core.h"
#include "types.h"
#include "combinators.h"
#include "parsers_internal.h"
#include "pow10.h"

namespace parse {

/**
 * Parser that always succeeds
 */
inline constexpr auto success() {
    return parser([](auto& s) {
        return s.template return_success_emplace<none>();
    });
}

/**
 * Parser that always fails
 */
template <typename T = bool>
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
        if (s.empty()) {
            return s.template return_success_emplace<none>();
        }
        return s.template return_fail<none>();
    });
}

/**
 * Parser for any item
 */
inline constexpr auto any_item() {
    return parser([](auto& s) {
        if (s.empty())
            return s.template return_fail<decltype(s.front())>();

        auto front_iterator = s.position;
        s.advance(1);
        return s.return_success(s.get_at(front_iterator));
    });
}

/**
 * Parser for a single item
 */
template <typename ItemType>
inline constexpr auto item(ItemType&& i) {
    return parser([i = std::forward<ItemType>(i)](auto& s) {
        return internal::item(s, i);
    });
}

/**
 * Parser for a single item. Templated version.
 * This is faster than the above due to less copying.
 */
template <auto i>
inline constexpr auto item() {
    return parser([](auto& s) {
        return internal::item(s, i);
    });
}

/**
 * Parser for a single item
 */
template <typename ItemType>
inline constexpr auto not_item(ItemType&& i) {
    return parser([i = std::forward<ItemType>(i)](auto& s) {
        return internal::item<true>(s, i);
    });
}

/**
 * Parser for a single item. Templated version.
 * This is faster than the above due to less copying.
 */
template <auto i>
inline constexpr auto not_item() {
    return parser([](auto& s) {
        return internal::item<true>(s, i);
    });
}

/**
 * Parser for a single item matching the provided predicate.
 */
template <typename Pred>
inline constexpr auto item_if(Pred pred) {
    return parser([=](auto& s) {
        const auto& c = s.front();
        if (pred(c)) {
            s.advance(1);
            return s.return_success(c);
        } else {
            return s.template return_fail<std::decay_t<decltype(c)>>();
        }
    });
}

/**
 * Parser for the sequence described by [begin, end)
 */
template <typename Iterator>
inline constexpr auto sequence(Iterator begin, Iterator end) {
    return parser([=](auto& s) {
        return internal::sequence(s, std::distance(begin, end),
                                  [=](auto& i) {return algorithm::equal(begin, end, i);});
    });
}

/**
 * Parser for the sequence described by the template parameters.
 * This might give better performance due to less copying.
 */
template <auto v, auto... vs>
inline constexpr auto sequence() {
    return parser([](auto& s) {
        return internal::sequence(s, sizeof...(vs) + 1,
                                  [](auto& i){return algorithm::equal<v, vs...>(i);});
    });
}

/**
 * Parser for a sequence described by a string literal
 */
template <typename ItemType, size_t N, typename = types::enable_if_string_literal_type<ItemType>>
inline constexpr auto sequence(const ItemType (&seq)[N]) {
    return sequence(std::begin(seq), std::end(seq) - 1);
}

/**
 * Parser for any item contained in the set described by the provided string literal
 */
template <typename ItemType, size_t N, typename = types::enable_if_string_literal_type<ItemType>>
inline constexpr auto any_of(const ItemType (&seq)[N]) {
    return parse::item_if([b = std::begin(seq), e = std::end(seq) - 1](const auto& c) {
        return algorithm::contains(b, e, c);
    });
}

/**
 * Parser for any item contained in the set described by the provided template arguments
 */
template <auto v, auto... vs>
inline constexpr auto any_of() {
    return parse::item_if([](const auto& c) {
        return algorithm::contains<v, vs...>(c);
    });
}

/**
 * Parser for consuming n items
 */
inline constexpr auto consume(size_t n) {
    return parser([=](auto& s) {
        return internal::consume(s, n);
    });
}

/**
 * Templated version of `consume`.
 */
template <size_t N>
inline constexpr auto consume() {
    return parser([](auto& s) {
        return internal::consume(s, N);
    });
}

/**
 * Parser for consuming all items up until a certain item
 * Use boolean template parameter `Eat` to control whether or not to
 * consume the matched item, and `Include` to control whether to or not
 * to include the matched item in the result.
 */
template <bool Eat = true, bool Include = false, typename ItemType>
inline constexpr auto until_item(ItemType&& c) {
    return parser([c = std::forward<ItemType>(c)](auto& s) {
        return internal::until_item<Eat, Include>(s, c);
    });
}

/**
 * Templated version of `until_item`.
 */
template <auto item, bool Eat = true, bool Include = false>
inline constexpr auto until_item() {
    return parser([](auto& s) {
        return internal::until_item<Eat, Include>(s, item);
    });
}

/**
 * Parser for consuming all items up until a certain sequence described by [begin, end).
 * Use boolean template parameter `Eat` to control whether or not to
 * consume the matched sequence, and `Include` to controler whether or not to include
 * the matched sequence in the result.
 * This may be faster than using until(sequence()).
 */
template <bool Eat = true, bool Include = false, typename Iterator>
inline constexpr auto until_sequence(Iterator begin, Iterator end) {
    return parser([=](auto& s) {
        return internal::until_sequence<Eat, Include>(s,
                    [=](auto& b, auto& e) {return algorithm::search(b, e, begin, end);});
    });
}

/**
 * Parser for consuming all items up until the given string literal.
 * Use boolean template parameter `Eat` to control whether or not to
 * consume the matched sequence, and `Include` to controler whether or not to include
 * the matched sequence in the result.
 * This is much faster than using `until(sequence())`.
 */
template <bool Eat = true,
          bool Include = false,
          typename ItemType, size_t N,
          typename = types::enable_if_string_literal_type<ItemType>>
inline constexpr auto until_sequence(const ItemType (&seq)[N]) {
    return until_sequence<Eat, Include>(std::begin(seq), std::end(seq) - 1);
}

/**
 * Parser for the rest of the sequence
 */
inline constexpr auto rest() {
    return parser([](auto& s) {
        auto start_pos = s.position;;
        s.set_position(s.end);
        return s.return_success(s.convert(start_pos, s.position));
    });
}

/**
 * Parser that consumes all items that matches the provided predicate.
 * This parser will never fail.
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
 * Parser that consumes all items in the set described by [start, end)
 */
template <typename Iterator>
inline constexpr auto while_in(Iterator start, Iterator end) {
    return while_predicate([=](const auto& val){return algorithm::find(start, end, val) != end;});
}

/**
 * Parser that consumes all items contained in the given string literal
 */
template <typename ItemType, size_t N, typename = types::enable_if_string_literal_type<ItemType>>
inline constexpr auto while_in(const ItemType (&items)[N]) {
    return while_in(std::begin(items), std::end(items) - 1);
}

/**
 * Parser that consumes all items contained in set described by the template parameters.
 * This might be faster than the above.
 */
template <auto v, auto... vs>
inline constexpr auto while_in() {
    return while_predicate([](const auto& val){
        return algorithm::contains<v, vs...>(val);
    });
}

/**
 * Parser that consumes all items between the two supplied sequences.
 * Use template parameter `Nested` to decide whether to support nested matchings or not,
 * and template parameter `Eat` to decide whether to include the matching sequence in the
 * result or not.
 */
template <bool Nested = false, bool Eat = true, typename ItemType, size_t NStart, size_t NEnd>
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

    return internal::between_general<NStart-1, NEnd-1, Nested, Eat>(start, end, compar_start, compare_end);
}

/**
 * Parser that consumes all items between the two supplied items.
 */
template <bool Nested = false, bool Eat = true, typename ItemType>
inline constexpr auto between_items(const ItemType start, const ItemType end) {
    constexpr auto compare_single = [](const auto iterator, auto, const auto& toCompare) {
        return *iterator == toCompare;
    };
    return internal::between_general<1, 1, Nested, Eat>(start, end, compare_single, compare_single);
}

/**
 * Create a custom parser.
 * custom_parser should be a callable with the following signature
 * std::pair<Iterator, std::optional<Result>>(Iterator position, Iterator End)
 * where the first element is the new iterator position, and the second the result, where
 * and empty optional signals a failed parse.
 */
template <typename Parser>
inline constexpr auto custom(Parser custom_parser) {
    return parser([=](auto& s) {
        return internal::custom(s, custom_parser(s.position, s.end));
    });
}

/**
 * Create a custom parser.
 * custom_parser should be a callable with the following signature
 * std::pair<Iterator, std::optional<Result>>(Iterator position, Iterator End, State& state)
 * where the first element is the new iterator position, and the second the result, where
 * and empty optional signals a failed parse.
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
 * Consider using `integer` or `floating` instead, as it they are constexpr, and performance
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
 * Parses negative numbers if the template argument is signed.
 */
template <typename Integral = int, bool IncludeDoubleDivisor = false>
inline constexpr auto integer() {
    auto p = fold<true, true>(item_if([](const auto& c) {return c >= '0' && c <= '9';}),
                              std::pair<Integral, unsigned>(0, 1), [](auto& r, auto&& c) {
        if constexpr (IncludeDoubleDivisor) {
            r.second *= 10;
        }
        r.first = r.first * 10 + c - '0';
    });

    auto res_parser = [p](bool neg) {
        return p >>= [=](auto&& res) {
            if (neg) res.first = -res.first;
            if constexpr (IncludeDoubleDivisor) {
                return mreturn(std::move(res));
            } else {
                return mreturn(res.first);
            }
        };
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
 * Use template parameter AllowScientific to enable/disable support for scientific notation.
 */
template <bool AllowScientific = true, typename FloatType = double>
inline constexpr auto floating() {
    constexpr auto floating_part = integer() >>= [](auto&& n) {
        auto dec = item<'.'>() >> integer<unsigned int, true>();
        return (dec >>= [=](auto&& p) {
            // ((0 <= n) - (n < 0)) returns -1 for n < 0 otherwise 1
            return mreturn(n + (((0 <= n) - (n < 0)) * (int(p.first)) / FloatType(p.second)));
        }) || mreturn_emplace<FloatType>(n);
    };

    if constexpr (AllowScientific) {
        return floating_part >>= [](auto&& d) {
            auto exp = any_of<'e', 'E'>() >> integer();
            return (exp >>= [=](auto&& e) {
                return mreturn(d * internal::pow_table<FloatType>::pow(e));
            }) || mreturn(d);
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

#endif // PARSER_PARSERS_H
