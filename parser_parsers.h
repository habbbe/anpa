#ifndef PARSER_PARSERS_H
#define PARSER_PARSERS_H

#include "parser_core.h"

namespace parse {

/**
 * Parser that always succeeds
 */
inline constexpr auto success() {
    return parser([=](auto &s) {
        return return_success(s, std::decay_t<decltype(s)>{});
    });
}

/**
 * Parser that always fails
 */
inline constexpr auto fail() {
    return parser([=](auto &s) {
        return return_fail_type<bool>(s);
    });
}

/**
 * Parser for the empty string
 */
inline constexpr auto empty() {
    return parser([=](auto &s) {
        if (s.rest.empty()) {
            return return_success(s, true);
        }

        return return_fail_type<bool>(s);
    });
}

/**
 * Parser for a single character
 */
inline constexpr auto token(const char c) {
    return parser([=](auto &s) {
        if (s.empty() || s.front() != c)
            return return_fail<char>(s);
        auto front = s.front();
        remove_prefix(s.rest, 1);
        return return_success(s, front);
    });
}

/**
 * Parser for a string
 */
template <int N>
inline constexpr auto string(const char (&str)[N]) {
    return parser([=](auto &s) {
        if (s.rest.length() < N-1 || s.rest.compare(0, N-1, str) != 0)
            return return_fail(s);
        auto res = s.rest.substr(0, N-1);
        remove_prefix(s.rest, N-1);
        return return_success(s, res);
    });
}

/**
 * Parser for consuming n characters
 */
inline constexpr auto consume(unsigned int n) {
    return parser([=](auto &s) {
        if (s.length() < n)
            return return_fail(s);
        auto res = s.rest.substr(0, n);
        remove_prefix(s.rest, n);
        return return_success(s, res);
    });
}

/**
 * Parser for consuming all characters up until a certain character
 * Use boolean template parameter `Eat` to control whether or not the
 */
template <typename CharType, bool Eat = true>
inline constexpr auto until_token(const CharType c) {
    return parser([=](auto &s) {
        if (auto pos = s.rest.find_first_of(c); pos != std::decay_t<decltype(s.rest)>::npos) {
            constexpr auto end = Eat ? 0 : 1;
            auto res = s.rest.substr(0, pos + end);
            remove_prefix(s.rest, pos+1);
            return return_success(s, res);
        } else {
            return return_fail(s);
        }
    });
}

/**
 * Parser for the rest for the string
 */
inline constexpr auto rest() {
    return parser([=](auto &s) {
        auto res = s.rest;
        s.rest = "";
        return return_success(s, res);
    });
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
        for (size_t i = 0; i < s.length(); ++i) {
            if (contains(str, s.rest[i])) {
                ++found;
            } else {
                break;
            }
        }
        if (found > 0) {
            auto result = s.rest.substr(0, found);
            remove_prefix(s.rest, found);
            return return_success(s, result);
        }
        return return_fail(s);
    });
}

/**
 * Parser that consumes all characters between two of the specified character
 */
template <typename CharType, bool eat = true>
inline constexpr auto match(const CharType c) {
    return parser([=](auto &s) {
        if (!s.rest.empty() && s.rest.front() == c) {
            if (auto pos = s.find_first_of(c, 1); pos != std::decay_t<decltype(s.rest)>::npos) {
                constexpr size_t start = eat ? 1 : 0;
                constexpr size_t end = eat ? -1 : 1;
                auto res = s.rest.substr(start, pos+end);
                remove_prefix(s.rest, pos + 1);
                return return_success(s, res);
            }
        }
        return return_fail(s);
    });
}

/**
 * Parser that consumes all characters between the two specified characters.
 * Use template parameter `Nested` to decide whether to support nested matchings or not,
 * and template parameter `Eat` to decide whether to include the matching characters in the
 * result or not.
 */
template <typename CharType, bool Nested = false, bool Eat = true>
inline constexpr auto match_two(const CharType first, const CharType last) {

    return parser([=](auto &s) {
        if (s.rest.empty() || s.rest.front() != first)
            return return_fail(s);

        auto len = s.rest.length();
        size_t to_match = 0;
        for (size_t i = 1; i<len; ++i) {
            if (s.rest[i] == last) {
                if (to_match == 0) {
                    constexpr size_t start = Eat ? 1 : 0;
                    constexpr size_t end = Eat ? -1 : 1;
                    auto res = s.rest.substr(start, i+end);
                    remove_prefix(s.rest, i + 1);
                    return return_success(s, res);
                } else if constexpr (Nested) {
                    --to_match;
                }
            } else if (s[i] == first) {
                if constexpr (Nested) {
                    ++to_match;
                }
            }
        }
        return return_fail(s);
    });
}

// CONVENIENCE PARSERS
/**
 * Parser for an integer
 */
template <bool Signed = true>
inline constexpr auto integer() {
    constexpr auto integer_parser = [](bool addMinus) {
        while_in("0123456789") >>= [addMinus](auto&& res) {
            std::string str(res);
            return mreturn(std::stoi("-" + str));
        };
    };
    if constexpr (Signed) {
        return succeed(string("-")) >>= [](bool hasMinus) {
            return integer_parser(hasMinus);
        };
    } else {
        return integer_parser(false);
    }
}

/**
 * Parser for whitespace
 */
inline constexpr auto whitespace() {
    return while_in(" \t\n\r\f");
}

}

#endif // PARSER_PARSERS_H
