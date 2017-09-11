#ifndef TOKEN_PARSER_H
#define TOKEN_PARSER_H

#include <optional>
#include <string_view>
#include "parser_combinators.h"

namespace parse {

/*
 * Parser for the empty string
 */
inline constexpr auto empty() {
    return [=](std::string_view s) {
        if (s.empty())
            return std::make_pair(s, std::optional{default_result_type{}});
        return std::make_pair(s, std::make_optional<default_result_type>());
    };
}

/*
 * Parser for a single character
 */
inline constexpr auto token(const char c) {
    return [=](std::string_view s) {
        if (s.empty() || s.front() != c)
            return std::make_pair(s, std::optional<char>{});
        auto front = s.front();
        s.remove_prefix(1);
        return std::make_pair(s, std::optional{front});
    };
}

/*
 * Parser for a string
 */
template <int N>
inline constexpr auto string(const char (&str)[N]) {
    return [=](std::string_view s) {
        if (s.empty() || s.length() < N-1 || s.compare(0, N-1, str) != 0)
            return std::make_pair(s, std::optional<default_result_type>{});
        auto res = s.substr(0, N-1);
        s.remove_prefix(N-1);
        return std::make_pair(s, std::make_optional<default_result_type>(res));
    };
}

/*
 * Parser for consuming n characters
 */
inline constexpr auto consume(unsigned int n) {
    return [=](std::string_view s) {
        if (s.length() < n) {
            return std::make_pair(s, std::optional<default_result_type>{});
        }
        auto res = s.substr(0, n);
        s.remove_prefix(n);
        return std::make_pair(s, std::make_optional<default_result_type>(res));
    };
}

/*
 * Parser for consuming all characters up until a certain character
 */
template <bool eat = true>
inline constexpr auto until_token(const char c) {
    return [=](std::string_view s) {
        if (auto pos = s.find_first_of(c); pos != std::string_view::npos) {
            constexpr auto end = eat ? 0 : 1;
            auto res = s.substr(0, pos + end);
            s.remove_prefix(pos+1);
            return std::make_pair(s, std::make_optional<default_result_type>(res));
        } else {
            return std::make_pair(std::string_view{}, std::optional<default_result_type>{});
        }
    };
}

/*
 * Parser that consumes all characters in the given set
 */
inline constexpr auto while_in(std::string_view set) {
    return [=](std::string_view s) {
        size_t found = 0;
        for (size_t i = 0; i < s.length(); ++i) {
            if (auto pos = set.find_first_of(s[i]); pos != std::string_view::npos) {
                ++found;
            } else {
                break;
            }
        }
        if (found > 0) {
            auto result = s.substr(0, found);
            s.remove_prefix(found);
            return std::make_pair(s, std::make_optional<default_result_type>(result));
        }
        return std::make_pair(s, std::optional<default_result_type>{});
    };
}

/*
 * Parser that consumes all characters between two of the specified character
 */
template <bool eat = true>
inline constexpr auto match(const char c) {
    return [=](std::string_view s) {
        if (!s.empty() && s.front() == c) {
            if (auto pos = s.find_first_of(c, 1); pos != std::string_view::npos) {
                constexpr size_t start = eat ? 1 : 0;
                constexpr size_t end = eat ? -1 : 1;
                auto res = s.substr(start, pos+end);
                s.remove_prefix(pos + 1);
                return std::make_pair(s, std::make_optional<default_result_type>(res));
            }
        }
        return std::make_pair(s, std::optional<std::string_view>{});
    };
}

/*
 * Parser that consumes all characters between matching brackets
 */
template <char c, bool eat = true>
inline constexpr auto match_bracket() {
    constexpr auto matching = []() {
        if constexpr (c == '(') return ')';
        else if constexpr (c == '<') return '>';
        else if constexpr (c == '[') return ']';
        else if constexpr (c == '{') return '}';
        else static_assert(c - c, "Matching not supported");
    }();

    return [](std::string_view s) {
        if (s.empty() || s.front() != c)
            return std::make_pair(s, std::optional<default_result_type>{});

        auto len = s.length();
        size_t to_match = 0;
        for (size_t i = 1; i<len; ++i) {
            if (s[i] == matching) {
                if (to_match == 0) {
                    constexpr size_t start = eat ? 1 : 0;
                    constexpr size_t end = eat ? -1 : 1;
                    auto res = s.substr(start, i+end);
                    s.remove_prefix(i + 1);
                    return std::make_pair(s, std::make_optional<default_result_type>(res));
                } else {
                    --to_match;
                }
            } else if (s[i] == c) {
                ++to_match;
            }
        }
        return std::make_pair(s, std::optional<default_result_type>{});
    };
}

// CONVENIENCE PARSERS
/*
 * Parser for an integer
 */
template <bool Signed = true>
constexpr auto integer() {
    auto unsigned_parser = while_in("0123456789") >>= [](auto res) {
        std::string str(res);
        return mreturn(std::stoi(str));
    };
    if constexpr (Signed) {
        return succeed(string("-")) + unsigned_parser;
    } else {
        return unsigned_parser;
    }
}

/*
 * Parser for whitespace
 */
constexpr auto whitespace() {
    return while_in(" \t\n\r\f");
}

}

#endif // TOKEN_PARSER_H
