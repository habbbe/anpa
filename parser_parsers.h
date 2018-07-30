#ifndef PARSER_PARSERS_H
#define PARSER_PARSERS_H

#include <stdlib.h>
#include <string>
#include "parser_core.h"
#include "parser_combinators.h"
#include <charconv>

namespace parse {

/**
 * Parser that always succeeds
 */
inline constexpr auto success() {
    return parser([](auto &) {
        return return_success(true);
    });
}

/**
 * Parser that always fails
 */
template <typename T = bool>
inline constexpr auto fail() {
    return parser([](auto &) {
        return return_fail<T>();
    });
}

/**
 * Parser for the empty sequence
 */
inline constexpr auto empty() {
    return parser([](auto &s) {
        if (s.empty()) {
            return return_success(true);
        }
        return return_fail<bool>();
    });
}

/**
 * Parser for any item
 */
inline constexpr auto any_item() {
    return parser([](auto &s) {
        if (s.empty())
            return return_fail<decltype(s.front())>();
        auto front = s.front();
        s.advance(1);
        return return_success(front);
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
                return return_success(front);
            }
        }
        return return_fail<ItemType>();
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
        if (auto result = custom_parser(s.position, s.end); result) {
            s.set_position(result->first);
            return return_success(result->second);
        } else {
            return return_fail<decltype(result->second)>();
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
        if (auto result = custom_parser(s.position, s.end, s.state); result) {
            s.set_position(result->first);
            return return_success(result->second);
        } else {
            return return_fail<decltype(result->second)>();
        }
    });
}

/**
 * Parser for a sequence
 */
template <typename ItemType, size_t N>
inline constexpr auto sequence(const ItemType (&seq)[N]) {
    return parser([=](auto &s) {
        constexpr auto seq_length = N - 1;
        if (s.has_at_least(seq_length) && std::equal(s.position, s.position + seq_length, seq)) {
            s.advance(seq_length);
            return return_success(s.convert(seq_length));
        } else {
            return return_fail_default(s);
        }
    });
}

/**
 * Parser for consuming n items
 */
inline constexpr auto consume(unsigned int n) {
    return parser([=](auto &s) {
        if (s.has_at_least(n)) {
            auto result = s.convert(n);
            s.advance(n);
            return return_success(result);
        }
        return return_fail_default(s);
    });
}

/**
 * Parser for consuming all items up until a certain item
 * Use boolean template parameter `Eat` to control whether or not to
 * include the matched item in the result.
 */
template <typename ItemType, bool Eat = true>
inline constexpr auto until_item(const ItemType c) {
    return parser([=](auto &s) {
        if (auto pos = std::find(s.position, s.end, c); pos != s.end) {
            auto end_iterator_with_token = pos + 1;
            auto res = s.convert(Eat ? pos : end_iterator_with_token);
            s.set_position(end_iterator_with_token);
            return return_success(res);
        } else {
            return return_fail_default(s);
        }
    });
}

/**
 * Parser for consuming all items up until a certain sequence
 * Use boolean template parameter `Eat` to control whether or not to
 * include the matched sequence in the result.
 * This is much faster than using until(sequence()).
 */
template <typename ItemType, size_t N, bool Eat = true>
inline constexpr auto until_sequence(const ItemType (&seq)[N]) {
    return parser([&](auto &s) {
        if (auto pos = std::search(s.position, s.end, seq, seq+N-1); pos != s.end) {
            constexpr auto seq_length = N - 1;
            auto end_iterator_including_seq = pos + seq_length;
            auto res = s.convert(Eat ? pos : end_iterator_including_seq);
            s.set_position(end_iterator_including_seq);
            return return_success(res);
        } else {
            return return_fail_default(s);
        }
    });
}

/**
 * Parser for the rest of the sequence
 */
inline constexpr auto rest() {
    return parser([](auto &s) {
        auto res = s.convert(s.end);
        s.set_position(s.end);
        return return_success(res);
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
 * Parser that consumes all items in the given set
 */
template <typename ItemType, size_t N>
inline constexpr auto while_in(const ItemType (&items)[N]) {
    return parser([=](auto &s) {
        constexpr auto contains = [&](auto &val) {
            auto end = items + N - 1;
            return std::find(items, end, val) != end;
        };

        auto i = s.position;
        auto res = std::find_if_not(s.position, s.end, [](auto &i) {return contains(i);});
        s.set_position(res);
        return return_success(s.convert(res));
    });
}

// General matching algorithm with supplied equality functions.
template <size_t StartLength, size_t EndLength, bool Nested = false, bool Eat = true, typename Start, typename End, typename EqualStart, typename EqualEnd>
static inline constexpr auto between_general(Start start, End end, EqualStart equal_start, EqualEnd equal_end) {
    return parser([=](auto &s) {
        if (s.empty() || !equal_start(s.position, s.position + StartLength, start))
            return return_fail_default(s);

        size_t to_match = 0;
        for (auto i = s.position + StartLength; i != s.end - EndLength;) {
            if (equal_end(i, i + EndLength, end)) {
                if (to_match == 0) {
                    auto begin_iterator = s.position + (Eat ? StartLength : 0);
                    auto result_end = i + (Eat ? 0 : EndLength);
                    s.set_position(i + EndLength);
                    return return_success(s.convert(begin_iterator, result_end));
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
        return return_fail_default(s);
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
        return std::equal(begin, end, toCompare) != end;
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
    return parser([=](auto &s) {
        bool negate = std::is_signed_v<Integral> && !s.empty() && s.front() == '-';
        auto start_iterator = s.position + (negate ? 1 : 0);
        auto [new_pos, result] = parse_integer<Integral>(start_iterator, s.end);
        if (new_pos != start_iterator) {
            s.set_position(new_pos);
            return return_success(result * (negate ? -1 : 1));
        } else {
            return return_fail<Integral>();
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
            return return_success(result);
        } else {
            return return_fail<Number>();
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
