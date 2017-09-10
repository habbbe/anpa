#ifndef TOKEN_PARSER_H
#define TOKEN_PARSER_H

#include <optional>
#include <string_view>

namespace parse {

/*
 * Parser for a single character
 */
constexpr auto token(const char c) {
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
constexpr auto string(std::string_view str) {
    return [=](std::string_view s) {
        auto len = str.length();
        if (s.empty() || s.length() < len || s.compare(0, len, str) != 0)
            return std::make_pair(s, std::optional<std::string_view>{});
        auto res = s.substr(0, len);
        s.remove_prefix(len);
        return std::make_pair(s, std::optional{res});
    };
}

/*
 * Parser for consuming n characters
 */
constexpr auto consume(unsigned int n) {
    return [=](std::string_view s) {
        if (s.length() >= n) {
            auto res = s.substr(0, n);
            s.remove_prefix(n);
            return std::make_pair(s, std::optional{res});
        } else {
            return std::make_pair(s, std::optional<std::string_view>{});
        }
    };
}

/*
 * Parser for consuming all characters up until a certain character
 */
template <bool eat = true>
constexpr auto until_token(const char c) {
    return [=](std::string_view s) {
        if (auto pos = s.find_first_of(c); pos != std::string_view::npos) {
            constexpr auto end = eat ? 0 : 1;
            auto res = s.substr(0, pos + end);
            s.remove_prefix(pos+1);
            return std::make_pair(s, std::optional{res});
        } else {
            return std::make_pair(std::string_view{}, std::optional<std::string_view>{});
        }
    };
}

/*
 * Parser that consumes all characters in the given set
 */
constexpr auto while_in(std::string_view set) {
    return [=](std::string_view s) {
        if (auto pos = s.find_first_not_of(set, 0); pos != std::string_view::npos) {
            auto result = s.substr(0, pos);
            s.remove_suffix(pos);
            return std::make_pair(s, std::optional{result});
        }
        return std::make_pair(s, std::optional<std::string_view>{});
    };
}

/*
 * Parser that consumes all characters between two of the specified character
 */
template <bool eat = true>
constexpr auto match(const char c) {
    return [=](std::string_view s) {
        if (!s.empty() && s.front() == c) {
            if (auto pos = s.find_first_of(c, 1); pos != std::string_view::npos) {
                constexpr size_t start = eat ? 1 : 0;
                constexpr size_t end = eat ? -1 : 1;
                auto res = s.substr(start, pos+end);
                s.remove_prefix(pos + 1);
                return std::make_pair(s, std::optional{res});
            }
        }
        return std::make_pair(s, std::optional<std::string_view>{});
    };
}

/*
 * Parser that consumes all characters between matching brackets
 */
template <char c, bool eat = true>
constexpr auto match_bracket() {
    constexpr auto matching = []() {
        if constexpr (c == '(') return ')';
        else if constexpr (c == '<') return '>';
        else if constexpr (c == '[') return ']';
        else if constexpr (c == '{') return '}';
        else static_assert(c - c, "Matching not supported");
    }();

    return [](std::string_view s) {
        if (s.empty() || s.front() != c)
            return std::make_pair(s, std::optional<std::string_view>{});

        auto len = s.length();
        size_t to_match = 0;
        for (size_t i = 1; i<len; ++i) {
            if (s[i] == matching) {
                if (to_match == 0) {
                    constexpr size_t start = eat ? 1 : 0;
                    constexpr size_t end = eat ? -1 : 1;
                    auto res = s.substr(start, i+end);
                    s.remove_prefix(i + 1);
                    return std::make_pair(s, std::optional{res});
                } else {
                    --to_match;
                }
            } else if (s[i] == c) {
                ++to_match;
            }
        }
        return std::make_pair(s, std::optional<std::string_view>{});
    };
}

// COMBINATORS

/*
 * Put a value in the parser monad
 */
template <typename T>
constexpr auto mreturn(T value) {
    return [=](std::string_view s) {
        return std::make_pair(s, std::optional{value});
    };
}

/*
 * Combine two parsers, ignoring the result of the first one
 */
template <typename Parser1, typename Parser2>
constexpr auto operator>>(Parser1 p1, Parser2 p2) {
    using return_type = decltype(p2({}));
    return [=](std::string_view s) {
        auto result = p1(s);
        if (result.second) {
            return p2(result.first);
        } else {
            return return_type(result.first, {});
        }
    };
}

/*
 * Monadic bind of two parsers
 */
template <typename Parser, typename Fun>
constexpr auto operator>>=(Parser p, Fun f) {
    using p_return_type = decltype(*p({}).second);
    using return_type = decltype(f(p_return_type{})({}));
    return [=](std::string_view s) {
        auto result = p(s);
        if (result.second) {
            return f(*result.second)(result.first);
        } else {
            return return_type(result.first, {});
        }
    };
}

/*
 * Make a parser a non consuming parser
 */
template <typename Parser>
constexpr auto no_consume(Parser p) {
    return [=](std::string_view s) {
        auto result = p(s);
        return std::make_pair(s, result.second);
    };
}

/*
 * Combine two parsers so that that both will be tried before failing
 */
template <typename Parser>
constexpr auto operator||(Parser p1, Parser p2) {
    using return_type = decltype(p1(std::string_view{}));
    return [=](std::string_view s) {
        if (auto res1 = p1(s); res1.second) {
            return res1;
        } else if (auto res2 = p2(s); res2.second) {
            return res2;
        } else {
            return return_type(s, {});
        }
    };
}

/*
 * Convert a parser to a parser that always succeeds
 */
template <typename Parser>
constexpr auto succeed(Parser p) {
    using return_type = decltype(*p(std::string_view{}).second);
    return [=](std::string_view s) {
        auto result = p(s);
        if (result.second) {
            return result;
        }
        return std::make_pair(s, std::optional{return_type{}});
    };
}

/*
 * Create a parser that applies a parser until it fails and saves
 * the results that satisfies the criteria to `it`
 */
template <typename Parser, typename OutputIt, typename Predicate>
constexpr auto many_if(Parser p, OutputIt it, Predicate pred) {
    return [=](std::string_view s) mutable {
        auto result = p(s);
        while (result.second && pred(*result.second)) {
            *it++ = *result.second;
            result = p(result.first);
        }
        return std::make_pair(result.first, std::optional(it));
    };
}

/*
 * Create a parser that applies a parser until it fails and saves
 * the results to `it`
 */
template <typename Parser, typename OutputIt>
constexpr auto many(Parser p, OutputIt it) {
    return many_if(p, it, []([[maybe_unused]]auto a){return true;});
}

}

#endif // TOKEN_PARSER_H
