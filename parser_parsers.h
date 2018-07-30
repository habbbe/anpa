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
 * Parser for the empty string
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
 * Parser for any character
 */
inline constexpr auto any_token() {
    return parser([](auto &s) {
        if (s.empty())
            return return_fail<char>();
        auto front = s.front();
        s.advance(1);
        return return_success(front);
    });
}

/**
 * Parser for a single character
 */
inline constexpr auto token(const char c) {
    return parser([=](auto &s) {
        if (!s.empty()) {
            if (auto front = s.front(); front == c) {
                s.advance(1);
                return return_success(front);
            }
        }
        return return_fail<char>();
    });
}

/**
 * Parser for a string
 */
template <typename CharType, int N>
inline constexpr auto string(const CharType (&str)[N]) {
    return parser([=](auto &s) {
        constexpr auto str_length = N - 1;
        if (s.length() < str_length || !std::equal(s.position, s.position + str_length, str)) {
            return return_fail_string(s);
        }
        s.advance(str_length);
        return return_success(s.convert(str_length));
    });
}

/**
 * Parser for consuming n characters
 */
inline constexpr auto consume(unsigned int n) {
    return parser([=](auto &s) {
        if (s.length() >= n) {
            auto result = s.convert(n);
            s.advance(n);
            return return_success(result);
        }
        return return_fail_string(s);
    });
}

/**
 * Parser for consuming all characters up until a certain character
 * Use boolean template parameter `Eat` to control whether or not to
 * include the matched token in the result.
 */
template <typename CharType, bool Eat = true>
inline constexpr auto until_token(const CharType c) {
    return parser([=](auto &s) {
        if (auto pos = std::find(s.position, s.end, c); pos != s.end) {
            auto end_iterator_with_token = pos + 1;
            auto res = s.convert(Eat ? pos : end_iterator_with_token);
            s.set_position(end_iterator_with_token);
            return return_success(res);
        } else {
            return return_fail_string(s);
        }
    });
}

/**
 * Parser for consuming all characters up until a certain string
 * Use boolean template parameter `Eat` to control whether or not to
 * include the matched string in the result.
 * This is much faster than using until(string()).
 */
template <size_t N, bool Eat = true>
inline constexpr auto until_string(const char (&str)[N]) {
    return parser([&](auto &s) {
        if (auto pos = std::search(s.position, s.end, str, str+N-1); pos != s.end) {
            constexpr auto str_length = N - 1;
            auto end_iterator_including_string = pos + str_length;
            auto res = s.convert(Eat ? pos : end_iterator_including_string);
            s.set_position(end_iterator_including_string);
            return return_success(res);
        } else {
            return return_fail_string(s);
        }
    });
}

/**
 * Parser for the rest for the string
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
    return parse::until_string("\r\n") || parse::until_token('\r') || parse::until_token('\n');
}

/**
 * Parser for the rest of the line
 */
inline constexpr auto end_of_line() {
    return parse::string("\r\n") || parse::string("\r") || parse::string("\n");
}

/**
 * Parser that consumes all characters in the given set
 */
template <typename CharType, size_t N>
inline constexpr auto while_in(const CharType (&str)[N]) {
    return parser([=](auto &s) {
        constexpr auto contains = [&](auto &val) {
            auto end = str + N - 1;
            return std::find(str, end, val) != end;
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
            return return_fail_string(s);

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
        return return_fail_string(s);
    });
}

/**
 * Parser that consumes all characters between the two supplied strings.
 * Use template parameter `Nested` to decide whether to support nested matchings or not,
 * and template parameter `Eat` to decide whether to include the matching characters in the
 * result or not.
 */
template <bool Nested = false, bool Eat = true, typename CharType, size_t NStart, size_t NEnd>
inline constexpr auto between_strings(const CharType (&start)[NStart], const CharType (&end)[NEnd]) {

    // Use faster comparison when the string is only one character long
    [[maybe_unused]] constexpr auto compare_single = [](auto begin, auto, auto toCompare) {
        return *begin == *toCompare;
    };
    [[maybe_unused]] constexpr auto compare_str = [](auto begin, auto end, auto toCompare) {
        return std::equal(begin, end, toCompare) != end;
    };

    constexpr auto compare_start = [=]() {
        if constexpr (NStart - 1 == 1) return compare_single; else return compare_str;
    }();
    constexpr auto compare_end = [=]() {
        if constexpr (NEnd - 1 == 1) return compare_single; else return compare_str;
    }();

    return between_general<NStart-1, NEnd-1, Nested, Eat>(start, end, compare_start, compare_end);
}

/**
 * Parser that consumes all characters between the two supplied characters
 */
template <bool Nested = false, bool Eat = true, typename CharType>
inline constexpr auto between_tokens(const CharType start, const CharType end) {
    constexpr auto compare_single = [](auto iterator, auto, auto toCompare) {
        return *iterator == toCompare;
    };
    return between_general<1, 1, Nested, Eat>(start, end, compare_single, compare_single);
}

/**
 * Parser that consumes all characters between two of the supplied character
 */
template <typename CharType, bool Nested = false, bool Eat = true>
inline constexpr auto between_token(const CharType c) {
    return between_tokens<Nested, Eat>(c, c);
}

// CONVENIENCE PARSERS

template <typename Integral, typename Iterator>
inline constexpr auto parse_integer(Iterator begin, Iterator end) {
    constexpr auto is_digit = [](char c) {
        return c <= '9' && c >= '0';
    };
    Integral result = 0;
    for (; begin != end; ++begin) {
        if (is_digit(*begin)) {
            result = (*begin - '0') + result * 10;
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
