#ifndef PARSER_CORE_H
#define PARSER_CORE_H

#include <optional>
//#include <variant>

namespace parse {

//template <typename T>
//using variant = std::variant<const char *, T>;

// Convenience function for returning a failed parse with state and type of result.
template <typename Res>
static constexpr auto return_fail() {
    return std::optional<std::decay_t<Res>>{};
//    return variant<std::decay_t<Res>>(std::in_place_index_t<0>(), error);
}

// Convenience function for returning a succesful parse.
template <typename Res>
static constexpr auto return_success(Res&& res) {
    return std::make_optional(std::forward<Res>(res));
//    return variant<std::decay_t<Res>>(std::in_place_index_t<1>(), std::forward<Res>(res));
}

// Convenience function for returning a succesful parse.
template <typename T, typename... Res>
static constexpr auto return_success_forward(Res&&... res) {
    return std::make_optional<T>(std::forward<Res>(res)...);
//    return variant<T>(std::in_place_index_t<1>(), std::forward<Res>(res)...);
}

/**
 * Check if the result is a successful one
 */
template <typename Result>
static constexpr bool has_result(const Result& r) {
    return r.operator bool();
//    return r.index() == 1;
}

/**
 * Unpack the result to the underlying successful result.
 * Note: Has undefined behavior if the result is not successful. Check has_result() before.
 */
template <typename Result>
static constexpr decltype(auto) get_result(const Result& r) {
    return *r;
//    return std::get<1>(r);
}

/**
 * Unpack the result to the underlying error.
 * Note: Has undefined behavior if the result is successful. Check !has_result() before.
 */
template <typename Result>
static constexpr auto get_error(const Result&) {
    return false;
//    return std::get<0>(r);
}

template <typename P>
struct parser;


/**
 * Class for the parser state.
 */
struct parser_state_simple {
    const std::string_view text;
    size_t position = 0;

    template <typename StringType>
    constexpr parser_state_simple(const StringType& text) : text{std::string_view(text)} {}
    constexpr auto front() const {return text[position];}
    constexpr auto length() const {return text.length() - position; }
    constexpr auto empty() const {return length() < 1;}
    constexpr auto advance(size_t n) {position += n;}
private:
    parser_state_simple(const parser_state_simple &other) = delete;
};


/**
 * Class for the parser state. Contains the string to be parsed, along
 * with the user provided state
 */
template <typename UserState>
struct parser_state: public parser_state_simple {
    UserState& user_state;
    template <typename StringType>
    constexpr parser_state(const StringType& text, UserState& state) : parser_state_simple{text}, user_state{state} {}

    template <typename StringType>
    constexpr parser_state(const parser_state& other, StringType& text) : parser_state_simple{text}, user_state{other.user_state} {}
};

/**
 * Monadic bind for the parser
 */
template <typename Parser, typename F>
static constexpr auto operator>>=(Parser p, F f) {
    return parser([=](auto &s) {
        if (auto result = p(s); has_result(result)) {
            return f(get_result(result))(s);
        } else {
            using new_return_type = std::decay_t<decltype(get_result(f(get_result(result))(s)))>;
            return return_fail<new_return_type>();
        }
    });
}

/**
 * Lifts a type to the parser monad by forwarding the provided arguments to its constructor.
 */
template <typename T, typename... Args>
constexpr auto mreturn_forward(Args&&... args) {
    return parser([=](auto &) {
        return return_success_forward<T>(std::move(args)...);
    });
}

/**
 * Lift a value to the parser monad
 */
template <typename T>
constexpr auto mreturn(T&& t) {
    return parser([=](auto &) {
        return return_success(std::move(t));
    });
}

/**
 * Monadic parser
 */
template <typename P>
struct parser {

    // The meat of the parser. A function that takes a parser state and returns an optional result
    P p;

    constexpr parser(P p) : p{p} {}

    template <typename State>
    constexpr auto operator()(State &s) const {
        return p(s);
    }

    /**
     * Begin parsing with the user supplied string and state.
     * The result is a std::pair with the position of the first unparsed token as first
     * element and the result of the parse as the second.
     */
    template <typename StringType, typename State>
    constexpr auto parse_with_state(const StringType &string, State &user_state) const {
        parser_state state(string, user_state);
        auto res = p(state);
        return std::make_pair(state.position, std::move(res));
    }

    /**
     * Begin parsing with the user supplied string
     * The result is a std::pair with the position of the first unparsed token as first
     * element and the result of the parse as the second.
     */
    template <typename StringType>
    constexpr auto parse(const StringType &string) const {
        auto state = parser_state_simple(string);
        auto res = p(state);
        return std::make_pair(state.position, std::move(res));
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

}

#endif // PARSER_CORE_H
