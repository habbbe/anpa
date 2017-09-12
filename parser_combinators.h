#ifndef PARSER_COMBINATORS_H
#define PARSER_COMBINATORS_H

#include <string_view>
#include <optional>
#include <utility>

namespace parse {

using default_string_type = std::string_view;
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
template <typename Monad1, typename Monad2>
inline constexpr auto operator>>(Monad1 m1, Monad2 m2) {
    return m1 >>= [=] ([[maybe_unused]] auto v) {
        return m2;
    };
}

/*
 * Combine two parsers, ignoring the result of the second one
 */
template <typename Monad1, typename Monad2>
inline constexpr auto operator<<(Monad1 m1, Monad2 m2) {
    using return_type = decltype(m1({}));
    return [=](std::string_view s) {
        auto result = m1(s);
        if (result.second) {
            auto result2 = m2(result.first);
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
    return [=](std::string_view s) {
        auto result = p(s);
        using return_type = std::decay_t<decltype(f(*result.second)(result.first))>;
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
 * Combine two parsers so that the second will be tried before failing.
 * If the two parsers return different types the return value will be ignored.
 */
template <typename Parser1, typename Parser2>
inline constexpr auto operator||(Parser1 p1, Parser2 p2) {
    using return_type_1 = std::decay_t<decltype(*p1({}).second)>;
    using return_type_2 = std::decay_t<decltype(*p2({}).second)>;
    return [=](std::string_view s) {
        if constexpr (std::is_same_v<return_type_1, return_type_2>) {
            if (auto res1 = p1(s); res1.second) {
                return res1;
            } else if (auto res2 = p2(s); res2.second) {
                return res2;
            } else {
                using return_type = decltype(p1(std::string_view{}));
                return return_type(s, {});
            }
        } else {
            if (auto res1 = p1(s); res1.second) {
                return std::make_pair(res1.first, std::optional(default_result_type{}));
            } else if (auto res2 = p2(s); res2.second) {
                return std::make_pair(res2.first, std::optional(default_result_type{}));
            } else {
                return std::make_pair(s, std::optional<default_result_type>{});
            }
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

/*
 * Parser that always succeeds
 */
inline constexpr auto success() {
    return [=](std::string_view s) {
        return std::make_pair(s, std::optional(default_result_type{}));
    };
}

/*
 * Transform a parser to a parser that fails on a successful, but empty result
 */
template <typename Parser>
inline constexpr auto not_empty(Parser p) {
    return [=](std::string_view s) {
       if (auto result = p(s); result.second && !std::empty(*result.second)) {
            return std::make_pair(result.first, std::make_optional(*result.second));
       }
       using return_type = std::decay_t<decltype(*p(std::string_view{}).second)>;
       return std::make_pair(s, std::optional<return_type>{});
    };
}

/*
 * Transform a parser to a parser that always succeeds
 */
template <typename Parser>
inline constexpr auto succeed(Parser p) {
    return [=](std::string_view s) {
        auto result = p(s);
        using return_type = std::decay_t<decltype(*result.second)>;
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
template <typename Container, typename Parser, typename Predicate>
inline constexpr auto many_if_value(Parser p, Predicate pred) {
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
inline constexpr auto many_value(Parser p) {
    return many_if_value<Container>(p, []([[maybe_unused]]auto a){return true;});
}

template <typename T, typename Monad>
inline constexpr auto lift(Monad m) {
    return m >>= [] (auto &&r) {
        return mreturn_emplace<T>(r);
    };
}

template <typename T, typename Monad1, typename Monad2>
inline constexpr auto lift2(Monad1 m1, Monad2 m2) {
    return m1 >>= [=] (auto &&r1) {
        return m2 >>= [=](auto &&r2) {
            return mreturn_emplace<T>(r1, r2);
        };
    };
}

template <typename T, typename Monad1, typename Monad2, typename Monad3>
inline constexpr auto lift3(Monad1 m1, Monad2 m2, Monad3 m3) {
    return m1 >>= [=] (auto &&r1) {
        return m2 >>= [=](auto &&r2) {
            return m3 >>= [=] (auto &&r3) {
                return mreturn_emplace<T>(r1, r2, r3);
            };
        };
    };
}

}
#endif // PARSER_COMBINATORS_H
