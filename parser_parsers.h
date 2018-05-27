#ifndef PARSER_PARSERS_H
#define PARSER_PARSERS_H

#include <stdlib.h>
#include <string>
#include "parser_core.h"
#include "parser_combinators.h"

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
template <int N>
inline constexpr auto string(const char (&str)[N]) {
    return parser([=](auto &s) {
        if (s.length() < N-1 || s.text.compare(s.position, N-1, str) != 0)
            return return_fail<decltype(s.text)>();
        auto res = s.substr(s.position, N-1);
        s.advance(N-1);
        return return_success(res);
    });
}

/**
 * Parser for consuming n characters
 */
inline constexpr auto consume(unsigned int n) {
    return parser([=](auto &s) {
        if (s.length() < n)
            return return_fail<decltype(s.text)>();
        auto res = s.substr(s.position, n);
        s.advance(n);
        return return_success(res);
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
        if (auto pos = s.text.find_first_of(c, s.position); pos != std::decay_t<decltype(s.text)>::npos) {
            auto parsed_length_excluding_token = pos - s.position;
            auto parsed_length_including_token = parsed_length_excluding_token + 1;
            auto res = s.substr(s.position, (Eat ? parsed_length_excluding_token : parsed_length_including_token));
            s.advance(parsed_length_including_token);
            return return_success(res);
        } else {
            return return_fail<decltype(s.text)>();
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
        if (auto pos = s.text.find(str, s.position); pos != std::decay_t<decltype(s.text)>::npos) {
            constexpr auto str_length = N - 1;
            auto parsed_size_excluding_string = pos - s.position;
            auto parsed_size_including_string = parsed_size_excluding_string + str_length;
            auto res = s.substr(s.position, Eat ? parsed_size_excluding_string : parsed_size_including_string);
            s.advance(parsed_size_including_string);
            return return_success(res);
        } else {
            return return_fail<decltype(s.text)>();
        }
    });
}

/**
 * Parser for the rest for the string
 */
inline constexpr auto rest() {
    return parser([](auto &s) {
        auto res = s.substr(s.position, std::string_view::npos);
        s.advance(s.length());
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
    return parser([&](auto &s) {
        constexpr auto contains = [](auto& str, auto c) {
            for (size_t i = 0; i < N-1; ++i) {
                if (str[i] == c) return true;
            }
            return false;
        };

        size_t found = 0;
        for (size_t i = s.position; i < s.end; ++i) {
            if (contains(str, s.text[i])) {
                ++found;
            } else {
                break;
            }
        }

        if (found > 0) {
            auto result = s.substr(s.position, found);
            s.advance(found);
            return return_success(result);
        } else {
            return return_fail<decltype(s.text)>();
        }
    });
}

// General matching algorithm with supplied equality functions.
template <size_t StartLength, size_t EndLength, bool Nested = false, bool Eat = true, typename Start, typename End, typename EqualStart, typename EqualEnd>
static inline constexpr auto between_general(Start start, End end, EqualStart equal_start, EqualEnd equal_end) {
    return parser([=](auto &s) {
        if (s.empty() || !equal_start(s.text, s.position, StartLength, start))
            return return_fail<decltype(s.text)>();

        auto len = s.end;
        size_t to_match = 0;
        for (size_t i = s.position + StartLength; i<=len-EndLength;) {
            if (equal_end(s.text, i, EndLength, end)) {
                if (to_match == 0) {
                    size_t start_index = s.position + (Eat ? StartLength : 0);
                    size_t size = Eat ? i - StartLength : i + EndLength;
                    auto res = s.substr(start_index, size);
                    s.advance(i + EndLength);
                    return return_success(res);
                } else if (Nested) {
                    --to_match;
                    i += EndLength;
                }
            } else if (Nested && equal_start(s.text, i, StartLength, start)) {
                ++to_match;
                i += StartLength;
            } else {
                ++i;
            }
        }
        return return_fail<decltype(s.text)>();
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
    [[maybe_unused]] constexpr auto compare_single = [](auto &s, auto start_index, auto, auto toCompare) {
        return s[start_index] == *toCompare;
    };
    [[maybe_unused]] constexpr auto compare_str = [](auto &s, auto start_index, auto length, auto toCompare) {
        return !s.compare(start_index, length, toCompare);
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
    constexpr auto compare_single = [](auto &s, auto start_index, auto, auto toCompare) {
        return s[start_index] == toCompare;
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

template <typename Integral>
inline constexpr std::pair<size_t, Integral> parse_integral(const char* str, size_t length) {
    constexpr auto is_digit = [](char c) {
        return c <= '9' && c >= '0';
    };
    Integral result = 0;
    size_t parsed = 0;
    for (; parsed < length; ++parsed, ++str) {
        if (is_digit(*str)) {
            result = (*str - '0') + result * 10;
        } else {
            break;
        }
    }
    return {parsed, result};
}

/**
 * Parser for an integer.
 * Parses negative numbers if the template argument is signed.
 */
template <typename Integral = int>
inline constexpr auto integer() {
    return parser([=](auto &s) {
        bool negate = std::is_signed_v<Integral> && !s.empty() && s.front() == '-';
        size_t start_pos = s.position + (negate ? 1 : 0);
        auto [parsed, result] = parse_integral<Integral>(s.text.data() + start_pos, s.end - start_pos);
        if (parsed > 0) {
            s.advance(parsed);
            return return_success(result * (negate ? -1 : 1));
        } else {
            return return_fail<Integral>();
        }
    });
}

/**
 * Parser for numbers using provided c call (strtol, strotof and friends).
 * This is fast, but doesn't behave correctly at the end of string types that aren't
 * null terminated in their underlying data array.
 */
template <typename Ret, typename ReturnType = Ret, typename CharType, typename... Base>
static inline constexpr auto convert_to_num(Ret (*c)(const CharType*, CharType**, Base...), Base... base) {
    return parser([=](auto &s) {
        const CharType *begin = s.text.data() + s.position;
        CharType* end;
        errno = 0;
        Ret result = c(begin, &end, base...);
        if (begin == end || errno == ERANGE ||
                   (std::is_integral<ReturnType>::value && sizeof(int) == sizeof(ReturnType) &&
                    (result < std::numeric_limits<ReturnType>::min() || result > std::numeric_limits<ReturnType>::max()))) {
            return return_fail<ReturnType>();
        } else {
            s.advance(end - begin);
            return return_success(ReturnType(result));
        }
    });
}

/**
 * Parse a double.
 * Note: Doesn't behave correctly at the end of non null terminated strings
 * Also consumes any whitespace before the number.
 */
inline constexpr auto double_fast() {
    return convert_to_num(std::strtod);
}

/**
 * Parse a float.
 * Note: Doesn't behave correctly at the end of non null terminated strings
 * Also consumes any whitespace before the number.
 */
inline constexpr auto float_fast() {
    return convert_to_num(std::strtof);
}

/**
 * Parser for whitespace
 */
inline constexpr auto whitespace() {
    return while_in(" \t\n\r\f");
}

}

#endif // PARSER_PARSERS_H
