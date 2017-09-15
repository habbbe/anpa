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

template <typename Res = result_string_type, typename S>
constexpr auto return_fail(S&& s) {
    return std::make_pair(std::forward<S>(s), std::optional<Res>{});
}

template <typename S, typename Res>
constexpr auto return_success(S&& s, Res&& res) {
    return std::make_pair(std::forward<S>(s), std::make_optional<Res>(std::forward<Res>(res)));
}

template <typename S, typename Res>
constexpr auto return_success_string(S&& s, Res&& res) {
    if constexpr (std::is_same_v<Res, result_string_type>) {
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
        return parse::mreturn_forward<T>(std::forward<Args>(args)...);
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

struct can_call_test
{
    template<typename F>
    static decltype(std::declval<F>().remove_prefix(0), std::true_type())
    f(int);

    template<typename F>
    static std::false_type
    f(...);
};

template<typename T>
constexpr bool is_string_view(T&&) { return decltype(can_call_test::f<T>(0)){}; }

template <typename S>
static constexpr auto remove_prefix(S &s, size_t size) {
    if constexpr (is_string_view(s)) {
         s.remove_prefix(size);
    } else {
        s.erase(0, size);
    }
}

/**
 * Combine two parsers and concatenate their result
 * This combinator is only defined if both parsers return all their
 * consumed input.
 */
//template <typename Parser1, typename Parser2>
//inline constexpr auto operator+(Parser1 p1, Parser2 p2) {
//    return parser([=](source_string_type s) {
//        if (auto result1 = p1(s); result1.second) {
//            if (auto result2 = p2(result1.first); result2.second) {
//                auto res = source_string_type(s.data(), result1.second->length() + result2.second->length());
//                return return_success(result2.first, res);
//            }
//        }
//        return return_type(s, {});
//    });
//}

/**
 * Combine two parsers so that the second will be tried before failing.
 * If the two parsers return different types the return value will be ignored.
 */
template <typename Parser1, typename Parser2>
inline constexpr auto operator||(Parser1 p1, Parser2 p2) {
    using R1 = decltype(*p1({}).second);
    using R2 = decltype(*p2({}).second);
    return parser([=](source_string_type s) {
        if constexpr (std::is_same_v<R1, R2>) {
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
                return return_success(res1.first, true);
            } else if (auto res2 = p2(s); res2.second) {
                return return_success(res2.first, true);
            } else {
                return return_fail<bool>(s);
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
        constexpr auto empty = [](auto&& v) {
            return std::empty(v);
        };
        if (auto result = p(s); result.second && !empty(*result.second)) {
            return return_success(result.first, *result.second);
        }
        using return_type = std::decay_t<decltype(*p(source_string_type{}).second)>;
        return std::make_pair(s, std::optional<return_type>{});
    });
}

/**
 * Transform a parser to a parser that always succeeds.
 * Will return true a result if the parser succeeded, and false otherwise
 */
template <typename Parser>
inline constexpr auto succeed(Parser p) {
    return parser([=](source_string_type s) {
        auto result = p(s);
        if (result.second) {
            return return_success(result.first, true);
        }
        return return_success(result.first, false);
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
        if (s.empty()) {
            auto r = return_success(s, true);
            return r;
        }

        auto r = return_fail<bool>(s);
        return r;
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
        remove_prefix(s, 1);
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
        remove_prefix(s, N-1);
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
        remove_prefix(s, n);
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
            remove_prefix(s, pos+1);
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
template <size_t N>
inline constexpr auto while_in(const char (&str)[N]) {
    return parser([=](source_string_type s) {
        constexpr auto contains = [](auto&& s, auto c) {
            for (size_t i = 0; i < N-1; ++i) {
                if (s[i] == c) return true;
            }
            return false;
        };

        size_t found = 0;
        for (size_t i = 0; i < s.length(); ++i) {
            if (contains(str, s[i])) {
                ++found;
            } else {
                break;
            }
        }
        if (found > 0) {
            auto result = s.substr(0, found);
            remove_prefix(s, found);
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
                remove_prefix(s, pos + 1);
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
                    remove_prefix(s, i + 1);
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
    constexpr auto integer_parser = [](bool addMinus) {
        while_in("0123456789") >>= [addMinus](auto res) {
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
constexpr auto whitespace() {
    return while_in(" \t\n\r\f");
}


// Base (fail) case
template <typename ResultType, typename StringType, typename F>
constexpr auto lift_or_rec(StringType &s,[[maybe_unused]] F f) {
        return return_fail<ResultType>(s);
}

// Compile time recursive resolver for lifting of arbitrary number of parsers
template <typename ResultType, typename StringType, typename F, typename Parser, typename... Parsers>
constexpr auto lift_or_rec(StringType &s, F f, Parser p, Parsers... ps) {
    if (auto res = p(s); res.second) {
            return return_success(s, f(*res.second));
    } else {
        return lift_or_rec<ResultType>(s, f, ps...);
    }
}

// Preparation step for or-lift
template <typename F, typename Parser, typename... Parsers>
constexpr auto lift_or_prep(F f, Parser p, Parsers... ps) {
    return parser([=](source_string_type s) {
        using return_type = std::decay_t<decltype(f(*p(s).second))>;
        return lift_or_rec<return_type>(s, f, p, ps...);
    });
}

/**
 * Lift the result of unary function to the parser monad after applying it to the first argument that succeeds.
 * The lifted function must provide an overload for every parser result type.
 */
template <typename F, typename Parser, typename... Parsers>
constexpr auto lift_or(F f, Parser p, Parsers... ps) {
    return lift_or_prep(f, p, ps...);
}

/**
 * Lift a type to the parser monad after applying the first successful argument to its constructor.
 * The constructor must provide an overload for every parser result type.
 */
template <typename T, typename Parser, typename... Parsers>
constexpr auto lift_or_value(Parser p, Parsers... ps) {
    constexpr auto construct = [](auto&&... args) {return T(std::forward<decltype(args)>(args)...);};
    return lift_or_prep(construct, p, ps...);
}

}

#endif // TOKEN_PARSER_H
