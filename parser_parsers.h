#ifndef PARSER_PARSERS_H
#define PARSER_PARSERS_H

#include "parser_core.h"

namespace parse {

/**
 * Parser that always succeeds
 */
inline constexpr auto success() {
    return parser([=](auto &s) {
        return return_success(std::decay_t<decltype(s)>{});
    });
}

/**
 * Parser that always fails
 */
template <typename T = bool>
inline constexpr auto fail() {
    return parser([=](auto &) {
        return return_fail<T>();
    });
}

/**
 * Parser for the empty string
 */
inline constexpr auto empty() {
    return parser([=](auto &s) {
        if (s.rest.empty()) {
            return return_success(true);
        }

        return return_fail<bool>();
    });
}

/**
 * Parser for any character
 */
inline constexpr auto any_token() {
    return parser([=](auto &s) {
        if (s.rest.empty())
            return return_fail<char>();
        auto front = s.rest.front();
        remove_prefix(s.rest, 1);
        return return_success(front);
    });
}

/**
 * Parser for a single character
 */
inline constexpr auto token(const char c) {
    return parser([=](auto &s) {
        if (s.rest.empty() || s.rest.front() != c)
            return return_fail<char>();
        auto front = s.rest.front();
        remove_prefix(s.rest, 1);
        return return_success(front);
    });
}

/**
 * Parser for a string
 */
template <int N>
inline constexpr auto string(const char (&str)[N]) {
    return parser([=](auto &s) {
        if (s.rest.length() < N-1 || s.rest.compare(0, N-1, str) != 0)
            return return_fail<decltype(s.rest)>();
        auto res = s.rest.substr(0, N-1);
        remove_prefix(s.rest, N-1);
        return return_success(res);
    });
}

/**
 * Parser for consuming n characters
 */
inline constexpr auto consume(unsigned int n) {
    return parser([=](auto &s) {
        if (s.rest.length() < n)
            return return_fail<decltype(s.rest)>();
        auto res = s.rest.substr(0, n);
        remove_prefix(s.rest, n);
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
        auto pos = s.rest.find_first_of(c);
        if (pos != std::decay_t<decltype(s.rest)>::npos) {
            constexpr auto end = Eat ? 0 : 1;
            auto res = s.rest.substr(0, pos + end);
            remove_prefix(s.rest, pos+1);
            return return_success(res);
        } else {
            return return_fail<decltype(s.rest)>();
        }
    });
}

/**
 * Parser for consuming all characters up until a certain string
 * Use boolean template parameter `Eat` to control whether or not to
 * include the matched string in the result
 */
template <size_t N, bool Eat = true>
inline constexpr auto until_string(const char (&str)[N]) {
    return parser([=](auto &s) {
        auto pos = s.rest.find(str);
        if (pos != std::decay_t<decltype(s.rest)>::npos) {
            auto end_pos = pos + (Eat ? 0 : N - 1);
            auto res = s.rest.substr(0, end_pos);
            remove_prefix(s.rest, pos + N - 1);
            return return_success(res);
        } else {
            return return_fail<decltype(s.rest)>();
        }
    });
}

/**
 * Parser for the rest for the string
 */
inline constexpr auto rest() {
    return parser([=](auto &s) {
        auto res = s.rest;
        s.rest = std::decay_t<decltype(s.rest)>();
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
        constexpr auto contains = [](auto& str, auto c) {
            for (size_t i = 0; i < N-1; ++i) {
                if (str[i] == c) return true;
            }
            return false;
        };

        size_t found = 0;
        for (size_t i = 0; i < s.rest.length(); ++i) {
            if (contains(str, s.rest[i])) {
                ++found;
            } else {
                break;
            }
        }
        if (found > 0) {
            auto result = s.rest.substr(0, found);
            remove_prefix(s.rest, found);
            return return_success(result);
        } else {
            return return_fail<decltype(s.rest)>();
        }
    });
}

// General matching algorithm with supplied equality functions.
template <size_t StartLength, size_t EndLength, bool Nested = false, bool Eat = true, typename Start, typename End, typename EqualStart, typename EqualEnd>
static inline constexpr auto between_general(Start start, End end, EqualStart equal_start, EqualEnd equal_end) {
    return parser([=](auto &s) {
        auto len = s.rest.length();
        if (len == 0 || !equal_start(s.rest, 0, StartLength, start))
            return return_fail<decltype(s.rest)>();

        size_t to_match = 0;
        for (size_t i = StartLength; i<=len-EndLength;) {
            if (equal_end(s.rest, i, EndLength, end)) {
                if (to_match == 0) {
                    constexpr size_t finalStart = Eat ? StartLength : 0;
                    size_t size = Eat ? i - StartLength : i + EndLength;
                    auto res = s.rest.substr(finalStart, size);
                    remove_prefix(s.rest, i + EndLength);
                    return return_success(s, res);
                } else if constexpr (Nested) {
                    --to_match;
                    i += EndLength;
                }
            } else if (Nested && equal_start(s.rest, i, StartLength, start)) {
                ++to_match;
                i += StartLength;
            } else {
                ++i;
            }
        }
        return return_fail<decltype(s.rest)>();
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

    constexpr auto compare_start = []() {
        if constexpr (NStart - 1 == 1) return compare_single; else return compare_str;
    }();
    constexpr auto compare_end = []() {
        if constexpr (NEnd - 1 == 1) return compare_single; else return compare_str;
    }();

    return between_general<NStart-1, NEnd-1, Nested, Eat>(start, end, compare_start, compare_end);
}

/**
 * Parser that consumes all characters between the two supplied characters
 */
template <bool Eat = true, typename CharType>
inline constexpr auto between_tokens(const CharType start, const CharType end) {
    constexpr auto compare_single = [](auto &s, auto start_index, auto, auto toCompare) {
        return s[start_index] == toCompare;
    };
    return between_general<1, 1, false, Eat>(start, end, compare_single, compare_single);
}

/**
 * Parser that consumes all characters between two of the supplied character
 */
template <typename CharType, bool eat = true>
inline constexpr auto between_token(const CharType c) {
    return between_tokens(c, c);
}

// CONVENIENCE PARSERS
/**
 * Parser for an integer
 */
template <bool Signed = true>
inline constexpr auto integer() {
    constexpr auto integer_parser = [](bool addMinus) {
        return while_in("0123456789") >>= [=](auto& res) {
            std::string str(res);
            if constexpr (Signed)
                return mreturn(std::stoi((addMinus ? "-" : "") + str));
            else
                return mreturn(unsigned(std::stoul(str)));

        };
    };
    if constexpr (Signed) {
        return succeed(token('-')) >>= [integer_parser](bool hasMinus) {
            return integer_parser(hasMinus);
        };
    } else {
        return integer_parser(false);
    }
}


/**
 * Parser for numbers using provided c call (strtol, strotof and friends).
 * This is fast, but doesn't behave correctly at the end of string types that aren't
 * null terminated in their underlying data array.
 */
template <typename Ret, typename ReturnType = Ret, typename CharType, typename... Base>
static inline constexpr auto convert_to_num(Ret (*c)(const CharType*, CharType**, Base...), Base... base) {
    return parser([=](auto &s) {
        const CharType *begin = s.rest.data();
        CharType* end;
        errno = 0;
        Ret result = c(begin, &end, base...);
        if (begin == end || errno == ERANGE ||
                   (std::is_integral<ReturnType>::value && sizeof(int) == sizeof(ReturnType) &&
                    (result < std::numeric_limits<ReturnType>::min() || result > std::numeric_limits<ReturnType>::max()))) {
            return return_fail<ReturnType>();
        } else {
            remove_prefix(s.rest, end - begin);
            return return_success(ReturnType(result));
        }
    });
}

/**
 * Parse an integer.
 * Note: Doesn't behave correctly at the end of non null terminated strings.
 * Also consumes any whitespace before the number.
 */
inline constexpr auto integer_fast(int base = 10) {
    return convert_to_num<long, int>(std::strtol, base);
}

/**
 * Parse an unsigned integer.
 * Note: Doesn't behave correctly at the end of non null terminated strings.
 * Also consumes any whitespace before the number.
 */
inline constexpr auto unsigned_integer_fast(int base = 10) {
    return convert_to_num<unsigned long, unsigned int>(std::strtoul, base);
}

/**
 * Parse a long integer.
 * Note: Doesn't behave correctly at the end of non null terminated strings.
 * Also consumes any whitespace before the number.
 */
inline constexpr auto long_fast(int base = 10) {
    return convert_to_num(std::strtol, base);
}

/**
 * Parse an unsigned long integer.
 * Note: Doesn't behave correctly at the end of non null terminated strings.
 * Also consumes any whitespace before the number.
 */
inline constexpr auto unsigned_long_fast(int base = 10) {
    return convert_to_num(std::strtoul, base);
}

/**
 * Parse a long long integer.
 * Note: Doesn't behave correctly at the end of non null terminated strings.
 * Also consumes any whitespace before the number.
 */
inline constexpr auto long_long_fast(int base = 10) {
    return convert_to_num(std::strtoll, base);
}

/**
 * Parse an unsigned long long integer.
 * Note: Doesn't behave correctly at the end of non null terminated strings.
 * Also consumes any whitespace before the number.
 */
inline constexpr auto unsigned_long_long_fast(int base = 10) {
    return convert_to_num(std::strtoull, base);
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
    return convert_to_num<float>(std::strtof);
}

/**
 * Parser for whitespace
 */
inline constexpr auto whitespace() {
    return while_in(" \t\n\r\f");
}

}

#endif // PARSER_PARSERS_H
