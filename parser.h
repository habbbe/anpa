#ifndef TOKEN_PARSER_H
#define TOKEN_PARSER_H

#include <optional>
#include <string_view>
#include <tuple>
#include "monad.h"

namespace parse {

// Convenience function for returning a pair with reference o state and a result.
template <typename State, typename Result>
static constexpr auto make_pair(State &s, Result&& res) {
    return std::pair<State&, Result>(s, std::forward<Result>(res));
}

// Convenience function for returning a failed parse. Default result to the string type used.
template <typename S>
static constexpr auto return_fail(S& s) {
    return make_pair(s, std::optional<typename std::decay_t<S>::string_type>{});
}

// Convenience function for returning a failed parse with state and type of result.
template <typename Res, typename S>
static constexpr auto return_fail_type(S& s) {
    return make_pair(s, std::optional<std::decay_t<Res>>{});
}

// Convenience function for returning a succesful parse.
template <typename S, typename Res>
static constexpr auto return_success(S& s, Res&& res) {
    return make_pair(s, std::make_optional(std::forward<Res>(res)));
}

template <typename P>
struct parser;

/**
 * Lifts a type to the parser monad by forwarding the provided arguments to its constructor.
 */
template <typename T, typename... Args>
constexpr auto mreturn_forward(Args&&... args) {
    return parser([=](auto &s) {
        return return_success(s, T(std::forward<Args>(args)...));
    });
}

/**
 * Lift a value to the parser monad
 */
template <typename T>
constexpr auto mreturn(T t) {
    return parser([=](auto &s) {
        return return_success(s, std::move(t));
    });
}

/**
 * Class for the parser state. Contains the rest of the string to be parsed.
 */
template <typename StringType>
struct parser_state_simple {
    using string_type = std::decay_t<StringType>;
    StringType rest;
    constexpr parser_state_simple(StringType rest) : rest{rest} {}
private:
    parser_state_simple(const parser_state_simple &other) {}
};

/**
 * Class for the parser state. Contains the rest of the string to be parsed, along
 * with the user provided state
 */
template <typename StringType, typename UserState>
struct parser_state: public parser_state_simple<StringType> {
    UserState& user_state;
    constexpr parser_state(StringType rest, UserState& state) : parser_state_simple<StringType>{rest}, user_state{state} {}
};

/**
 * Monadic parser
 */
template <typename P>
struct parser {

    // The meat of the parser. A function that takes a parser state and returns
    // a std::pair<State&, ResultType>
    P p;

    constexpr parser(P&& p) : p{std::forward<P>(p)} {}

    template <typename State>
    constexpr auto operator()(State &s) const {
        return p(s);
    }

    /**
     * Begin parsing with the user supplied string and state
     */
    template <typename StringType, typename State>
    auto parse_with_state(StringType&& string, State &user_state) const {
        auto state = parser_state(std::forward<StringType>(string), user_state);
        return p(state);
    }

    /**
     * Begin parsing with the user supplied string
     */
    template <typename StringType>
    auto parse(StringType&& string) const {
        auto state = parser_state_simple(std::forward<StringType>(string));
        return p(state);
    }

    /**
     * Class member of mreturn_forward. For general monad use.
     */
    template <typename T, typename... Args>
    static constexpr auto mreturn_forward(Args&&... args) {
        return parse::mreturn_forward<T>(std::forward<Args>(args)...);
    }

    /**
     * Class member of mreturn. For general monad use.
     */
    template <typename T>
    static constexpr auto mreturn(T&& v) {
        return parse::mreturn(std::forward<T>(v));
    }
};

/**
 * Monadic bind for the parser
 */
template <typename Parser, typename F>
static constexpr auto operator>>=(Parser p, F f) {
    return parser([=](auto &s) {
        auto result = p(s);
        if (result.second) {
            return f(*result.second)(result.first);
        } else {
            using new_return_type = std::decay_t<decltype(f(*result.second)(s))>;
            return new_return_type(result.first, {});
        }
    });
}

// For the below remove_prefix
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

/**
 * Helper function for removing prefix from std::string or std::string_view.
 */
template <typename S>
static constexpr auto remove_prefix(S &s, size_t size) {
    if constexpr (is_string_view(s)) {
         s.remove_prefix(size);
    } else {
        s.erase(0, size);
    }
}

/**
 * Combine two parsers so that the second will be tried before failing.
 * If the two parsers return different types the return value will be ignored.
 */
template <typename Parser1, typename Parser2>
inline constexpr auto operator||(Parser1 p1, Parser2 p2) {
    using R1 = decltype(*p1({}).second);
    using R2 = decltype(*p2({}).second);
    return parser([=](auto &s) {
        if constexpr (std::is_same_v<R1, R2>) {
            if (auto res1 = p1(s); res1.second) {
                return res1;
            } else if (auto res2 = p2(s); res2.second) {
                return res2;
            } else {
                using return_type = decltype(p1(s));
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
 * Make a parser non consuming
 */
template <typename Parser>
inline constexpr auto no_consume(Parser p) {
    return parser([=](auto &s) {
        auto str_copy = s.rest;
        auto res = p(s);
        s.rest = std::move(str_copy);
        return make_pair(s, res);
    });
}

/**
 * Parser that always succeeds
 */
inline constexpr auto success() {
    return parser([=](auto &s) {
        return return_success(s, std::decay_t<decltype(s)>{});
    });
}

/**
 * Make a parser that doesn't consume its input on failure
 */
template <typename Parser>
inline constexpr auto try_parser(Parser p) {
    return parser([=](auto &s) {
        // Make copy of rest of string to parse
        auto str_copy = s.rest;
        if (auto res = p(s); res.second) {
            return res;
        } else {
            using return_type = std::decay_t<decltype(*p(s).second)>;
            // Move back the old string
            s.rest = std::move(str_copy);
            return make_pair(s, std::optional<return_type>{});
        }
    });
}

/**
 * Transform a parser to a parser that fails on a successful, but empty result
 */
template <typename Parser>
inline constexpr auto not_empty(Parser p) {
    return parser([=](auto &s) {
        constexpr auto empty = [](auto& v) {
            return std::empty(v);
        };
        if (auto result = p(s); result.second && !empty(*result.second)) {
            return return_success(result.first, *result.second);
        }
        using return_type = std::decay_t<decltype(*p(s).second)>;
        return make_pair(s, std::optional<return_type>{});
    });
}

/**
 * Transform a parser to a parser that always succeeds.
 * Will return true a result if the parser succeeded, and false otherwise
 */
template <typename Parser>
inline constexpr auto succeed(Parser p) {
    return parser([=](auto &s) {
        auto result = p(s);
        if (result.second) {
            return return_success(result.first, true);
        }
        return return_success(result.first, false);
    });
}

/**
 * Apply a provided function to the state after evaluating a number of parsers.
 * The function provided shall have the state as its first argument (by reference) and
 * then a number of arguments that matches the number of parsers
 */
template <typename Fun, typename... Parsers>
inline constexpr auto apply_to_state(Fun f, Parsers&&...ps) {
    return parser([=](auto &s) {
        auto to_apply = [f, &s] (auto&&...vals) {
            return f(s.user_state, std::forward<decltype(vals)>(vals)...);
        };
        return monad::lift(to_apply, ps...)(s);
    });
}

/**
 * Create a parser that applies a parser until it fails or until the result doesn't satisfy
 * the user provided criteria.
 */
template <typename Container, typename Parser, typename Predicate>
inline constexpr auto many_if(Parser p, Predicate pred) {
    return parser([=](auto &s) {
        Container c;
        auto result = p(s);
        while (result.second && pred(*result.second)) {
            c.push_back(*result.second);
            result = p(result.first);
        }
        return make_pair(result.first, std::make_optional(c));
    });
}

/**
 * Create a parser that applies a parser until it fails
 */
template <typename Container, typename Parser>
inline constexpr auto many_value(Parser p) {
    return many_if<Container>(p, []([[maybe_unused]]auto a){return true;});
}

/**
 * Create a parser that applies a parser until it fails or until the result doesn't satisfy
 * the user provided criteria and stores the result in a container in the user provided state
 * which is accessed by the function acc.
 * The number of added elements is then returned.
 * Note that this parser doesn't fail, so you need to check the resulting number if nothing
 * was parsed.
 */
template <typename Parser, typename Predicate, typename Accessor>
inline constexpr auto many_if_to_state(Parser p, Predicate pred, Accessor acc) {
    return parser([=](auto &s) {
        auto result = p(s);
        std::decay_t<decltype(acc(s.user_state).size())> number_of_results = 0;
        while (result.second && pred(*result.second)) {
            auto &c = acc(s.user_state);
            c.emplace_back(*result.second);
            result = p(result.first);
            ++number_of_results;
        }
        return return_success(result.first, number_of_results);
    });
}

/**
 * Create a parser that applies a parser until it fails or until the result doesn't satisfy
 * the user provided criteria and stores the result in the user provided state.
 * Note that the user provided state needs to be a container.
 */
template <typename Parser, typename Predicate>
inline constexpr auto many_if_to_state(Parser p, Predicate pred) {
    return many_if_to_state(p, pred, [](auto &s) {return s;});
}

/**
 * Create a parser that applies a parser until it fails and stores the result in the user
 * provided state which is accessed by the function acc.
 */
template <typename Parser, typename Accessor>
inline constexpr auto many_to_state(Parser p, Accessor acc) {
    return many_if_to_state(p, [](auto &v) {return true;}, acc);
}

/**
 * Create a parser that applies a parser until it fails and stores the result in the user
 * provided state.
 * Note that the user provided state needs to be a container.
 */
template <typename Parser>
inline constexpr auto many_to_state(Parser p) {
    return many_to_state(p, [](auto &s) {return s;});
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
 */
template <bool eat = true>
inline constexpr auto until_token(const char c) {
    return parser([=](auto &s) {
        if (auto pos = s.rest.find_first_of(c); pos != std::decay_t<decltype(s.rest)>::npos) {
            constexpr auto end = eat ? 0 : 1;
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
template <size_t N>
inline constexpr auto while_in(const char (&str)[N]) {
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
template <bool eat = true>
inline constexpr auto match(const char c) {
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
 * Parser that consumes all characters between matching brackets
 */
template <char c, bool eat = true>
inline constexpr auto match_bracket() {

    return parser([](auto &s) {
        constexpr decltype(s.rest[0]) matching = []() {
            if constexpr (c == '(') return ')';
            else if constexpr (c == '<') return '>';
            else if constexpr (c == '[') return ']';
            else if constexpr (c == '{') return '}';
            else static_assert(c - c, "Matching not supported");
        }();

        if (s.rest.empty() || s.rest.front() != c)
            return return_fail(s);

        auto len = s.rest.length();
        size_t to_match = 0;
        for (size_t i = 1; i<len; ++i) {
            if (s.rest[i] == matching) {
                if (to_match == 0) {
                    constexpr size_t start = eat ? 1 : 0;
                    constexpr size_t end = eat ? -1 : 1;
                    auto res = s.rest.substr(start, i+end);
                    remove_prefix(s.rest, i + 1);
                    return return_success(s, res);
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

// Compile time recursive resolver for lifting of arbitrary number of parsers
template <typename State, typename F, typename Parser, typename... Parsers>
static inline constexpr auto lift_or_rec(State &s, F f, Parser p, Parsers... ps) {
    if (auto res = p(s); res.second) {
            return return_success(s, f(*res.second));
    } else {
        if constexpr (sizeof...(ps) > 0) {
            return lift_or_rec(s, f, ps...);
        } else {
            // All parsers failed
            return return_fail_type<std::decay_t<decltype(f(*std::get<1>(p(s))))>>(s);
        }
    }
}

/**
 * Lift the result of unary function to the parser monad after applying it to the first argument that succeeds.
 * The lifted function must provide an overload for every parser result type.
 */
template <typename F, typename Parser, typename... Parsers>
inline constexpr auto lift_or(F f, Parser p, Parsers... ps) {
    return parser([=](auto &s) {
        return lift_or_rec(s, f, p, ps...);
    });
}

/**
 * Lift a type to the parser monad after applying the first successful argument to its constructor.
 * The constructor must provide an overload for every parser result type.
 */
template <typename T, typename Parser, typename... Parsers>
inline constexpr auto lift_or_value(Parser p, Parsers... ps) {
    return parser([=](auto &s) {
        constexpr auto construct = [](auto&& arg) {return T(std::forward<decltype(arg)>(arg));};
        return lift_or_rec(s, construct, p, ps...);
    });
}

/**
 * Lift a type to the parser monad after applying the first successful argument to its constructor.
 * The constructor must provide an overload for every parser result type.
 * This version applies the constructor to a lazy argument
 */
template <typename T, typename Parser, typename... Parsers>
inline constexpr auto lift_or_value_from_lazy(Parser p, Parsers... ps) {
    return parser([=](auto &s) {
        constexpr auto construct = [](auto arg) {
            return T(arg());
        };
        return lift_or_rec(s, construct, p, ps...);
    });
}

}

#endif // TOKEN_PARSER_H
