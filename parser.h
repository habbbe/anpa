#ifndef TOKEN_PARSER_H
#define TOKEN_PARSER_H

#include <optional>
#include <string_view>
#include "monad.h"

namespace parse {

// The source string type
using source_string_type = std::string_view;

// The result string type
using result_string_type = std::string_view;

using optional_result_type = std::optional<result_string_type>;

auto empty_success = optional_result_type{result_string_type{}};

using return_type = std::pair<source_string_type, optional_result_type>;

template <typename Res = result_string_type>
constexpr auto return_fail(auto s) {
    return std::make_pair(s, std::optional<Res>{});
}

template <typename S, typename Res>
constexpr auto return_success(S&& s, Res&& res) {
    return std::make_pair(std::forward<S>(s), std::make_optional(std::forward<Res>(res)));
}

template <typename S, typename Res>
constexpr auto return_success_string(S&& s, Res&& res) {
    if constexpr (std::is_same_v<S, result_string_type>) {
        return return_success(std::forward<S>(s), std::forward<Res>(res));
    } else {
        return return_success(std::forward<S>(s), result_string_type(std::forward<Res>(res)));
    }
}

template <typename P>
struct parser;

/**
 * Monadic bind that constructs a value in place from its arguments
 */
template <typename T, typename... Args>
static constexpr auto mreturn_forward(Args&&... args) {
    return parser([=](source_string_type s) {
        return return_success(s, T(args...));
//        return return_type(s, lazy::make_lazy_forward<T>(args...));
    });
}

/**
 * Monadic bind
 */
template <typename T>
static constexpr auto mreturn(T&& v) {
    return parser([=](source_string_type s) {
        return return_success(s, v);
    });
}

/**
 * Monadic parser
 */
template <typename P>
struct parser {
    P p;

//    using result_type = decltype(*p({}).second);

    constexpr parser(P&& p) : p{std::forward<P>(p)} {}

    constexpr auto operator()(source_string_type s) const {
        return p(s);
    }

    template <typename T, typename... Args>
    static constexpr auto mreturn_forward(Args&&... args) {
        return parse::mreturn_forward<T>(args...);
    }

    template <typename T>
    static constexpr auto mreturn(T&& v) {
        return parse::mreturn(v);
    }

};

/**
 * Monadic bind for the parser
 */
template <typename Parser, typename F>
static constexpr auto operator>>=(Parser p, F f) {
    return parser([=](source_string_type s) {
        auto result = p(s);
        if (result.second) {
            return f(*result.second)(result.first);
        } else {
            using new_return_type = std::decay_t<decltype(f(*result.second)(s))>;
            return new_return_type(result.first, {});
        }
    });
}

/**
 * Combine two parsers and concatenate their result
 * This combinator is only defined if both parsers return all their
 * consumed input.
 */
template <typename Parser1, typename Parser2>
inline constexpr auto operator+(Parser1 p1, Parser2 p2) {
    return parser([=](source_string_type s) {
        if (auto result1 = p1(s); result1.second) {
            if (auto result2 = p2(result1.first); result2.second) {
                auto res = source_string_type(s.data(), result1.second->length() + result2.second->length());
                return return_success(result2.first, res);
            }
        }
        return return_type(s, {});
    });
}

/**
 * Combine two parsers so that the second will be tried before failing.
 * If the two parsers return different types the return value will be ignored.
 */
template <typename Parser1, typename Parser2>
inline constexpr auto operator||(Parser1 p1, Parser2 p2) {
    using return_type_1 = std::decay_t<decltype(*p1({}).second)>;
    using return_type_2 = std::decay_t<decltype(*p2({}).second)>;
    return parser([=](source_string_type s) {
        if constexpr (std::is_same_v<return_type_1, return_type_2>) {
            if (auto res1 = p1(s); res1.second) {
                return res1;
            } else if (auto res2 = p2(s); res2.second) {
                return res2;
            } else {
                using return_type = decltype(p1(source_string_type{}));
                return return_type(s, {});
            }
        } else {
            if (auto res1 = p1(s); res1.second) {
                return return_success(res1.first, empty_success);
            } else if (auto res2 = p2(s); res2.second) {
                return return_success(res2.first, empty_success);
            } else {
                return return_fail(s);
            }
        }
    });
}

/**
 * Make a parser a non consuming parser
 */
template <typename Parser>
inline constexpr auto no_consume(Parser p) {
    return parser([=](source_string_type s) {
        return std::make_pair(s, p(s).second);
    });
}

/**
 * Parser that always succeeds
 */
inline constexpr auto success() {
    return parser([=](source_string_type s) {
        return return_success_string(s, result_string_type{});
    });
}

/**
 * Transform a parser to a parser that fails on a successful, but empty result
 */
template <typename Parser>
inline constexpr auto not_empty(Parser p) {
    return parser([=](source_string_type s) {
       if (auto result = p(s); result.second && !std::empty(*result.second)) {
           return return_success(result.first, *result.second);
       }
       using return_type = std::decay_t<decltype(*p(source_string_type{}).second)>;
       return std::make_pair(s, std::optional<return_type>{});
    });
}

/**
 * Transform a parser to a parser that always succeeds
 */
template <typename Parser>
inline constexpr auto succeed(Parser p) {
    return parser([=](source_string_type s) {
        auto result = p(s);
        if (result.second) {
            return result;
        }
        using return_type = std::decay_t<decltype(*result.second)>;
        return std::make_pair(s, std::optional{return_type{}});
    });
}

/**
 * Create a parser that applies a parser until it fails and returns
 * the results that satisfies the criteria to
 */
template <typename Container, typename Parser, typename Predicate>
inline constexpr auto many_if_value(Parser p, Predicate pred) {
    return parser([=](source_string_type s) {
        Container c;
        auto result = p(s);
        while (result.second && pred(*result.second)) {
            c.push_back(*result.second);
            result = p(result.first);
        }
        return std::make_pair(result.first, std::optional(c));
    });
}

/**
 * Create a parser that applies a parser until it fails and saves
 * the results that satisfies the criteria to `it`
 */
template <typename Parser, typename OutputIt, typename Predicate>
inline constexpr auto many_if(Parser p, OutputIt it, Predicate pred) {
    return parser([=](source_string_type s) mutable {
        auto result = p(s);
        while (result.second && pred(*result.second)) {
            *it++ = *result.second;
            result = p(result.first);
        }
        return std::make_pair(result.first, std::optional(it));
    });
}

/**
 * Create a parser that applies a parser until it fails and saves
 * the results to `it`
 */
template <typename Parser, typename OutputIt>
inline constexpr auto many(Parser p, OutputIt it) {
    return many_if(p, it, []([[maybe_unused]]auto a){return true;});
}

/**
 * Create a parser that applies a parser until it fails
 */
template <typename Container, typename Parser, typename OutputIt>
inline constexpr auto many_value(Parser p) {
    return many_if_value<Container>(p, []([[maybe_unused]]auto a){return true;});
}

/**
 * Parser for the empty string
 */
inline constexpr auto empty() {
    return parser([=](source_string_type s) {
        if (s.empty())
            return return_success_string(s, s);
        return return_fail(s);
    });
}

/**
 * Parser for a single character
 */
inline constexpr auto token(const char c) {
    return parser([=](source_string_type s) {
        if (s.empty() || s.front() != c)
            return return_fail<char>(s);
        auto front = s.front();
        s.remove_prefix(1);
        return return_success(s, front);
    });
}

/**
 * Parser for a string
 */
template <int N>
inline constexpr auto string(const char (&str)[N]) {
    return parser([=](source_string_type s) {
        if (s.length() < N-1 || s.compare(0, N-1, str) != 0)
            return return_fail(s);
        auto res = s.substr(0, N-1);
        s.remove_prefix(N-1);
        return return_success_string(s, res);
    });
}

/**
 * Parser for consuming n characters
 */
inline constexpr auto consume(unsigned int n) {
    return parser([=](source_string_type s) {
        if (s.length() < n)
            return return_fail(s);
        auto res = s.substr(0, n);
        s.remove_prefix(n);
        return return_success_string(s, res);
    });
}

/**
 * Parser for consuming all characters up until a certain character
 */
template <bool eat = true>
inline constexpr auto until_token(const char c) {
    return parser([=](source_string_type s) {
        if (auto pos = s.find_first_of(c); pos != source_string_type::npos) {
            constexpr auto end = eat ? 0 : 1;
            auto res = s.substr(0, pos + end);
            s.remove_prefix(pos+1);
            return return_success_string(s, res);
        } else {
            return return_fail(s);
        }
    });
}

/**
 * Parser for the rest for the string
 */
inline constexpr auto rest() {
    return parser([=](source_string_type s) {
        return return_success_string(source_string_type{}, s);
    });
}

/**
 * Parser that consumes all characters in the given set
 */
inline constexpr auto while_in(source_string_type set) {
    return parser([=](source_string_type s) {
        size_t found = 0;
        for (size_t i = 0; i < s.length(); ++i) {
            if (auto pos = set.find_first_of(s[i]); pos != source_string_type::npos) {
                ++found;
            } else {
                break;
            }
        }
        if (found > 0) {
            auto result = s.substr(0, found);
            s.remove_prefix(found);
            return return_success_string(s, result);
        }
        return return_fail(s);
    });
}

/**
 * Parser that consumes all characters between two of the specified character
 */
template <bool eat = true>
inline constexpr auto match(const char c) {
    return parser([=](source_string_type s) {
        if (!s.empty() && s.front() == c) {
            if (auto pos = s.find_first_of(c, 1); pos != source_string_type::npos) {
                constexpr size_t start = eat ? 1 : 0;
                constexpr size_t end = eat ? -1 : 1;
                auto res = s.substr(start, pos+end);
                s.remove_prefix(pos + 1);
                return return_success_string(s, res);
            }
        }
        return return_fail(s);
    });
}

/**
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

    return parser([](source_string_type s) {
        if (s.empty() || s.front() != c)
            return return_fail(s);

        auto len = s.length();
        size_t to_match = 0;
        for (size_t i = 1; i<len; ++i) {
            if (s[i] == matching) {
                if (to_match == 0) {
                    constexpr size_t start = eat ? 1 : 0;
                    constexpr size_t end = eat ? -1 : 1;
                    auto res = s.substr(start, i+end);
                    s.remove_prefix(i + 1);
                    return return_success_string(s, res);
                } else {
                    --to_match;
                }
            } else if (s[i] == c) {
                ++to_match;
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

/**
 * Parser for whitespace
 */
constexpr auto whitespace() {
    return while_in(" \t\n\r\f");
}

}

#endif // TOKEN_PARSER_H
