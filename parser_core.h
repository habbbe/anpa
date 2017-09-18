#ifndef PARSER_CORE_H
#define PARSER_CORE_H

#include <optional>

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

}

#endif // PARSER_CORE_H
