#ifndef PARSER_CORE_H
#define PARSER_CORE_H

#include <optional>

namespace parse {

// Convenience function for returning a failed parse. Default result to the string type used.
template <typename State>
static constexpr auto return_fail() {
    return std::optional<typename std::decay_t<State>::string_type>{};
}

// Convenience function for returning a failed parse with state and type of result.
template <typename Res>
static constexpr auto return_fail_type() {
    return std::optional<std::decay_t<Res>>{};
}

// Convenience function for returning a succesful parse.
template <typename Res>
static constexpr auto return_success(Res&& res) {
    return std::make_optional(std::forward<Res>(res));
}

template <typename P>
struct parser;

/**
 * Lifts a type to the parser monad by forwarding the provided arguments to its constructor.
 */
template <typename T, typename... Args>
constexpr auto mreturn_forward(Args&&... args) {
    return parser([=](auto &) {
        return std::make_optional<T>(args...);
    });
}

/**
 * Lift a value to the parser monad
 */
template <typename T>
constexpr auto mreturn(T&& t) {
    return parser([=](auto &) {
        return std::make_optional(t);
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
    parser_state_simple(const parser_state_simple &other) = delete;
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

    // The meat of the parser. A function that takes a parser state and returns an optional result
    P p;

    constexpr parser(P&& p) : p{std::forward<P>(p)} {}

    template <typename State>
    constexpr auto operator()(State &s) const {
        return p(s);
    }

    /**
     * Begin parsing with the user supplied string and state.
     * The result is a std::pair with the unparsed string as first element and
     * the result of the parse as the second.
     */
    template <typename StringType, typename State>
    auto parse_with_state(StringType&& string, State &user_state) const {
        auto state = parser_state(std::forward<StringType>(string), user_state);
        auto res = p(state);
        return std::make_pair(std::move(state.rest), std::move(res));
    }

    /**
     * Begin parsing with the user supplied string
     * The result is a std::pair with the unparsed string as first element and
     * the result of the parse as the second.
     */
    template <typename StringType>
    auto parse(StringType&& string) const {
        auto state = parser_state_simple(std::forward<StringType>(string));
        auto res = p(state);
        return std::make_pair(std::move(state.rest), std::move(res));
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
        if (auto result = p(s)) {
            return f(*result)(s);
        } else {
            using new_return_type = std::decay_t<decltype(f(*result)(s))>;
            return new_return_type({});
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
constexpr bool is_string_view() { return decltype(can_call_test::f<T>(0)){}; }

/**
 * Helper function for removing prefix from std::string or std::string_view.
 */
template <typename S>
static constexpr auto remove_prefix(S &s, size_t size) {
    if constexpr (is_string_view<S>()) {
         s.remove_prefix(size);
    } else {
        s.erase(0, size);
    }
}

}

#endif // PARSER_CORE_H
