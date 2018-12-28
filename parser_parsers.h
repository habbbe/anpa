#ifndef PARSER_PARSERS_H
#define PARSER_PARSERS_H

#include <charconv>
#include "parser_core.h"
#include "parser_combinators.h"

namespace parse {

/**
 * Parser that always succeeds
 */
inline constexpr auto success() {
    return parser([](auto &s) {
        return s.return_success(true);
    });
}

/**
 * Parser that always fails
 */
template <typename T = bool>
inline constexpr auto fail() {
    return parser([](auto &s) {
        return s.template return_fail<T>();
    });
}

/**
 * Parser for the empty sequence
 */
inline constexpr auto empty() {
    return parser([](auto &s) {
        if (s.empty()) {
            return s.return_success(true);
        }
        return s.template return_fail<bool>();
    });
}

/**
 * Parser for any item
 */
inline constexpr auto any_item() {
    return parser([](auto &s) {
        if (s.empty())
            return s.template return_fail<decltype(s.front())>();
        auto front = s.front();
        s.advance(1);
        return s.return_success(front);
    });
}

/**
 * Parser for a single item
 */
template <typename ItemType>
inline constexpr auto item(const ItemType c) {
    return parser([=](auto &s) {
        if (!s.empty()) {
            if (auto front = s.front(); front == c) {
                s.advance(1);
                return s.return_success(front);
            }
        }
        return s.template return_fail<ItemType>();
    });
}

/**
 * Create a custom parser.
 * custom_parser should be a callable with the following signature
 * std::optional<std::pair<Iterator, Result>>(Iterator position, Iterator End)
 * where an empty optional signals a failed parse, and a value a successful parse; a pair with
 * the new iterator position and a result.
 */
template <typename Parser>
inline constexpr auto custom(Parser custom_parser) {
    return parser([=](auto &s) {
        if (auto result = custom_parser(s.position, s.end)) {
            s.set_position(result->first);
            return s.return_success(result->second);
        } else {
            return s.template return_fail<decltype(result->second)>();
        }
    });
}

/**
 * Create a custom parser.
 * custom_parser should be a callable with the following signature
 * std::optional<std::pair<Iterator, Result>>(Iterator position, Iterator End, State &state)
 * where an empty optional signals a failed parse, and a value a successful parse; a pair with
 * the new iterator position and a result.
 */
template <typename Parser>
inline constexpr auto custom_with_state(Parser custom_parser) {
    return parser([=](auto &s) {
        if (auto result = custom_parser(s.position, s.end, s.state)) {
            s.set_position(result->first);
            return s.return_success(result->second);
        } else {
            return s.template return_fail<decltype(result->second)>();
        }
    });
}

/**
 * Parser for the sequence described by [begin, end)
 */
template <typename Iterator>
inline constexpr auto sequence(Iterator begin, Iterator end) {
    return parser([=](auto &s) {
        auto size = std::distance(begin, end);
        auto orig_pos = s.position;
        if (s.has_at_least(size) && algorithm::equal(begin, end, orig_pos)) {
            s.advance(size);
            return s.return_success(s.convert(orig_pos, size));
        } else {
            return s.return_fail();
        }
    });
}

/**
 * Parser for a sequence
 */
template <typename ItemType, size_t N>
inline constexpr auto sequence(const ItemType (&seq)[N]) {
    return sequence(seq, seq + N - 1);
}

/**
 * Parser for consuming n items
 */
inline constexpr auto consume(size_t n) {
    return parser([=](auto &s) {
        if (s.has_at_least(n)) {
            auto result = s.convert(n);
            s.advance(n);
            return s.return_success(result);
        }
        return s.template return_fail(s);
    });
}

/**
 * Parser for consuming all items up until a certain item
 * Use boolean template parameter `Eat` to control whether or not to
 * include the matched item in the result.
 */
template <typename ItemType, bool Eat = true>
inline constexpr auto until_item(ItemType &&c) {
    return parser([c = std::forward<ItemType>(c)](auto &s) {
        if (auto pos = algorithm::find(s.position, s.end, c); pos != s.end) {
            auto end_iterator_with_token = pos + 1;
            auto res = s.convert(Eat ? pos : end_iterator_with_token);
            s.set_position(end_iterator_with_token);
            return s.return_success(res);
        } else {
            return s.return_fail();
        }
    });
}

/**
 * Parser for consuming all items up until a certain sequence described by [begin, end).
 * Use boolean template parameter `Eat` to control whether or not to
 * include the matched sequence in the result.
 * This is much faster than using until(sequence()).
 */
template <bool Eat = true, typename Iterator>
inline constexpr auto until_sequence(Iterator begin, Iterator end) {
    return parser([=](auto &s) {
        if (auto [pos, new_end] = algorithm::search(s.position, s.end, begin, end); pos != s.end) {
            auto res = s.convert(Eat ? pos : new_end);
            s.set_position(new_end);
            return s.return_success(res);
        } else {
            return s.template return_fail();
        }
    });
}

/**
 * Parser for consuming all items up until a certain sequence
 * Use boolean template parameter `Eat` to control whether or not to
 * include the matched sequence in the result.
 * This is much faster than using until(sequence()).
 */
template <bool Eat = true, typename ItemType, size_t N>
inline constexpr auto until_sequence(const ItemType (&seq)[N]) {
    return until_sequence<Eat>(seq, seq + N - 1);
}

/**
 * Parser for the rest of the sequence
 */
inline constexpr auto rest() {
    return parser([](auto &s) {
        auto res = s.convert(s.end);
        s.set_position(s.end);
        return s.return_success(res);
    });
}

/**
 * Parser for the rest of the line
 */
inline constexpr auto rest_of_line() {
    return parse::until_sequence("\r\n") || parse::until_item('\r') || parse::until_item('\n');
}

/**
 * Parser for the rest of the line
 */
inline constexpr auto end_of_line() {
    return parse::sequence("\r\n") || parse::item('\r') || parse::item('\n');
}

/**
 * Parser that consumes all items that matches the provided predicate
 */
template <typename Predicate>
inline constexpr auto while_predicate(Predicate predicate) {
    return parser([=](auto &s) {
        auto res = algorithm::find_if_not(s.position, s.end, predicate);
        auto result = s.convert(res);
        s.set_position(res);
        return s.return_success(result);
    });
}

/**
 * Parser that consumes all items in the set described by [start, end)
 */
template <typename Iterator>
inline constexpr auto while_in(Iterator start, Iterator end) {
    return while_predicate([=](const auto &val){return algorithm::find(start, end, val) != end;});
}

/**
 * Parser that consumes all items contained in the given array
 */
template <typename ItemType, size_t N>
inline constexpr auto while_in(const ItemType (&items)[N]) {
    return while_in(items, items + N - 1);
}

// General matching algorithm with supplied equality functions.
template <size_t StartLength, size_t EndLength, bool Nested = false, bool Eat = true, typename Start, typename End, typename EqualStart, typename EqualEnd>
static inline constexpr auto between_general(Start start, End end, EqualStart equal_start, EqualEnd equal_end) {
    return parser([=](auto &s) {
        if (s.empty() || !equal_start(s.position, s.position + StartLength, start))
            return s.template return_fail();

        size_t to_match = 0;
        for (auto i = s.position + StartLength; i != s.end - EndLength;) {
            if (equal_end(i, i + EndLength, end)) {
                if (to_match == 0) {
                    auto begin_iterator = s.position + (Eat ? StartLength : 0);
                    auto result_end = i + (Eat ? 0 : EndLength);
                    s.set_position(i + EndLength);
                    return s.return_success(s.convert(begin_iterator, result_end));
                } else if (Nested) {
                    --to_match;
                    i += EndLength;
                }
            } else if (Nested && equal_start(i, i+StartLength, start)) {
                ++to_match;
                i += StartLength;
            } else {
                ++i;
            }
        }
        return s.template return_fail();
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
        return algorithm::equal(begin, end, toCompare) != end;
    };

    constexpr auto compare_start = [=]() {
        if constexpr (NStart - 1 == 1) return compare_single; else return compare_seq;
    }();
    constexpr auto compare_end = [=]() {
        if constexpr (NEnd - 1 == 1) return compare_single; else return compare_seq;
    }();

    return between_general<NStart-1, NEnd-1, Nested, Eat>(start, end, compare_start, compare_end);
}

/**
 * Parser that consumes all items between the two supplied items.
 */
template <bool Nested = false, bool Eat = true, typename ItemType>
inline constexpr auto between_items(const ItemType start, const ItemType end) {
    constexpr auto compare_single = [](auto iterator, auto, auto toCompare) {
        return *iterator == toCompare;
    };
    return between_general<1, 1, Nested, Eat>(start, end, compare_single, compare_single);
}

/**
 * Parser that consumes all items between two of the supplied item
 */
template <typename ItemType, bool Nested = false, bool Eat = true>
inline constexpr auto between_item(const ItemType c) {
    return between_items<Nested, Eat>(c, c);
}

// CONVENIENCE PARSERS

template <typename Integral, typename Iterator>
inline constexpr auto parse_integer(Iterator begin, Iterator end) {
    constexpr auto is_digit = [](auto &c) {
        return c <= '9' && c >= '0';
    };
    Integral result = 0;
    while (begin != end) {
        if (is_digit(*begin)) {
            result = (*begin++ - '0') + result * 10;
        } else {
            break;
        }
    }
    return std::pair{begin, result};
}

/**
 * Parser for an integer.
 * Parses negative numbers if the template argument is signed.
 */
template <typename Integral = int>
inline constexpr auto integer() {
    return parser([](auto &s) {
        bool negate = std::is_signed_v<Integral> && !s.empty() && s.front() == '-';
        auto start_iterator = s.position + (negate ? 1 : 0);
        auto [new_pos, result] = parse_integer<Integral>(start_iterator, s.end);
        if (new_pos != start_iterator) {
            s.set_position(new_pos);
            return s.return_success(result * (negate ? -1 : 1));
        } else {
            return s.template return_fail<Integral>();
        }
    });
}

/**
 * Parse a number. Template parameter indicates the type to be parsed. Uses std::from_chars.
 * For integers, consider using integer instead, as it is constexpr and slightly faster.
 */
template <typename Number>
inline constexpr auto number() {
    return parser([](auto &s) {
        Number result;
        auto res = std::from_chars<Number>(s.position, s.end, result);
        if (res.errc != std::errc()) {
            s.set_position(res.ptr);
            return s.return_success(result);
        } else {
            return s.template return_fail<Number>();
        }
    });
}

/**
 * Parser for whitespace
 */
inline constexpr auto whitespace() {
    return while_in(" \t\n\r\f");
}

}

#endif // PARSER_PARSERS_H
