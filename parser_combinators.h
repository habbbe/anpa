#ifndef PARSER_COMBINATORS_H
#define PARSER_COMBINATORS_H

#include <string_view>
#include <optional>
#include <utility>

namespace parse {

using default_result_type = std::string_view;
using default_result_pair = std::pair<std::string_view, std::optional<default_result_type>>;
// COMBINATORS

/*
 * Put a value in the parser monad
 */
template <typename T>
inline constexpr auto mreturn(T value) {
    return [=](std::string_view s) {
        return std::make_pair(s, std::make_optional(value));
    };
}

/*
 * Put a value in the parser monad in place
 */
template <typename T, typename ... Args>
inline constexpr auto mreturn_emplace(Args&&... args) {
    return [=](std::string_view s) {
        return std::make_pair(s, std::make_optional<T>(args...));
    };
}

/*
 * Combine two parsers, ignoring the result of the first one
 */
template <typename Parser1, typename Parser2>
inline constexpr auto operator>>(Parser1 p1, Parser2 p2) {
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
 * Combine two parsers, ignoring the result of the second one
 */
template <typename Parser1, typename Parser2>
inline constexpr auto operator<<(Parser1 p1, Parser2 p2) {
    using return_type = decltype(p1({}));
    return [=](std::string_view s) {
        auto result = p1(s);
        if (result.second) {
            auto result2 = p2(result.first);
            if (result2.second) {
                return std::make_pair(result2.first, result.second);
            }
        }
        return return_type(s, {});
    };
}

/*
 * Monadic bind of two parsers
 */
template <typename Parser, typename Fun>
inline constexpr auto operator>>=(Parser p, Fun f) {
    using return_type = decltype(f(*p({}).second)({}));
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
 * Combine two parsers and concatenate their result
 * This combinator is only defined if both parsers return all their
 * consumed input.
 */
template <typename Parser1, typename Parser2>
inline constexpr auto operator+(Parser1 p1, Parser2 p2) {
    return [=](std::string_view s) {
        if (auto result1 = p1(s); result1.second) {
            if (auto result2 = p2(result1.first); result2.second) {
                auto res = std::string_view(s.data(), result1.second->length() + result2.second->length());
                return std::make_pair(result2.first, std::optional(res));
            }
        }
        return std::make_pair(s, std::optional<default_result_type>{});
    };
}

/*
 * Combine two parsers so that that both will be tried before failing
 */
template <typename Parser>
inline constexpr auto operator||(Parser p1, Parser p2) {
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
 * Combine two parsers ignoring the result
 */
template <typename Parser1, typename Parser2>
inline constexpr auto operator||(Parser1 p1, Parser2 p2) {
    return [=](std::string_view s) {
        if (auto res1 = p1(s); res1.second) {
            return std::make_pair(res1.first, std::optional(default_result_type{}));
        } else if (auto res2 = p2(s); res2.second) {
            return std::make_pair(res2.first, std::optional(default_result_type{}));
        } else {
            return std::make_pair(s, std::optional<default_result_type>{});
        }
    };
}

/*
 * Make a parser a non consuming parser
 */
template <typename Parser>
inline constexpr auto no_consume(Parser p) {
    return [=](std::string_view s) {
        auto result = p(s);
        return std::make_pair(s, result.second);
    };
}

inline constexpr auto success() {
    return [=](std::string_view s) {
        return std::make_pair(s, std::optional(default_result_type{}));
    };
}

/*
 * Convert a parser to a parser that always succeeds
 */
template <typename Parser>
inline constexpr auto succeed(Parser p) {
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
 * Create a parser that applies a parser until it fails and returns
 * the results that satisfies the criteria to
 */
template <typename Container, typename Parser, typename OutputIt, typename Predicate>
inline constexpr auto many_if_value(Parser p, OutputIt it, Predicate pred) {
    return [=](std::string_view s) {
        Container c;
        auto result = p(s);
        while (result.second && pred(*result.second)) {
            c.push_back(*result.second);
            result = p(result.first);
        }
        return std::make_pair(result.first, std::optional(c));
    };
}

/*
 * Create a parser that applies a parser until it fails and saves
 * the results that satisfies the criteria to `it`
 */
template <typename Parser, typename OutputIt, typename Predicate>
inline constexpr auto many_if(Parser p, OutputIt it, Predicate pred) {
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
inline constexpr auto many(Parser p, OutputIt it) {
    return many_if(p, it, []([[maybe_unused]]auto a){return true;});
}

/*
 * Create a parser that applies a parser until it fails and saves
 * the results to `it`
 */
template <typename Container, typename Parser, typename OutputIt>
inline constexpr auto many_if_value(Parser p, OutputIt it) {
    return many_if_value<Container>(p, it, []([[maybe_unused]]auto a){return true;});
}

}
#endif // PARSER_COMBINATORS_H
