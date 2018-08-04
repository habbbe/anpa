#ifndef PARSER_CORE_H
#define PARSER_CORE_H

#include <string_view>
#include "parser_result.h"
#include "parser_state.h"
#include "parser_settings.h"

namespace parse {

//template <typename T, typename ErrorType, typename Iterator, typename StringConversionFunction, typename S = void>
//using type = std::function<result<T, ErrorType>(std::conditional_t<std::is_void<S>::value, parser_state_simple<Iterator, StringConversionFunction> &, parser_state<Iterator, StringConversionFunction, S> &>)>;

/**
 * Apply a parser to a state and return the result.
 * This application unwraps arbitrary layers of callables so that one can
 * wrap the parser to enable recursion.
 */
template <typename P, typename S>
constexpr auto apply(P p, S &s) {
    if constexpr (std::is_invocable<P>::value) {
        return apply(p(), s);
    } else {
        return p(s);
    }
}

template <typename P>
struct parser;

/**
 * Monadic bind for the parser
 */
template <typename Parser, typename F>
static constexpr auto operator>>=(Parser p, F f) {
    return parser([=](auto &s) {
        if (auto result = apply(p, s); result) {
            return f(std::move(*result))(s);
        } else {
            using new_return_type = std::decay_t<decltype(*(f(*result)(s)))>;
            return s.template return_fail<new_return_type>();
        }
    });
}

/**
 * Lifts a type to the parser monad by forwarding the provided arguments to its constructor.
 */
template <typename T, typename... Args>
constexpr auto mreturn_forward(Args&&... args) {
    return parser([=](auto &s) {
        return s.template return_success_forward<T>(args...);
// The following doesn't compile with GCC
//    return parser([tup = std::tuple(std::forward<Args>(args)...)](auto &) {
//        return std::apply(return_success_forward<T, Args...>, tup);
    });
}

/**
 * Lift a value to the parser monad
 */
template <typename T>
constexpr auto mreturn(T&& t) {
    return parser([t = std::forward<T>(t)](auto &s) {
        return s.return_success(t);
    });
}

/**
 * Monadic parser
 */
template <typename P>
struct parser {

    // The meat of the parser. A function that takes a parser state and returns an optional result.
    // This could also be a lazy value, i.e. a callable object that returns what is described above.
    P p;

    constexpr parser(P p) : p{p} {}

    template <typename State>
    constexpr auto operator()(State &s) const {
        return apply(p, s);
    }

    /**
     * Begin parsing a sequence interpreted as [begin, end) with state,
     * using the supplied conversion function when returning sequence results.
     * The result is a std::pair with the position of the first unparsed token as first
     * element and the result of the parse as the second.
     */
    template <typename Iterator, typename State, typename ConversionFunction>
    constexpr auto parse_with_state(Iterator begin, Iterator end, State &user_state, ConversionFunction convert) const {
        parser_state state(begin, end, user_state, convert, parser_settings());
        auto res = apply(p, state);
        return std::make_pair(state.position, std::move(res));
    }

    /**
     * Begin parsing a sequence interpreted as [begin, end) with state,
     * using basic_string_view for sequence results.
     * The result is a std::pair with the position of the first unparsed token as first
     * element and the result of the parse as the second.
     */
    template <typename Iterator, typename State>
    constexpr auto parse_with_state(Iterator begin,
                                    Iterator end,
                                    State &user_state) const {
        return parse_with_state(begin,
                                end,
                                user_state,
                                string_view_convert);
    }

    /**
     * Begin parsing a sequence interpreted as [std::begin(sequence), std::end(sequence)) with state,
     * using basic_string_view for sequence results.
     * The result is a std::pair with the position of the first unparsed token as first
     * element and the result of the parse as the second.
     */
    template <typename SequenceType, typename State>
    constexpr auto parse_with_state(const SequenceType &sequence,
                                    State &user_state) const {
        return parse_with_state(std::begin(sequence),
                                std::end(sequence),
                                user_state);
    }

    /**
     * Begin parsing the sequence described by [begin, end),
     * using the supplied conversion function when returning sequence results.
     * The result is a std::pair with the position of the first unparsed token as first
     * element and the result of the parse as the second.
     */
    template <typename Iterator, typename ConversionFunction>
    constexpr auto parse(Iterator begin, Iterator end, ConversionFunction convert) const {
        auto state = parser_state_simple(begin, end, convert, parser_settings());
        auto res = apply(p, state);
        return std::make_pair(state.position, std::move(res));
    }

    /**
     * Begin parsing the sequence described by [begin, end),
     * using basic_string_view for sequence results.
     * The result is a std::pair with the position of the first unparsed token as first
     * element and the result of the parse as the second.
     */
    template <typename Iterator>
    constexpr auto parse(Iterator begin, Iterator end) const {
        return parse(begin, end, string_view_convert);
    }

    /**
     * Begin parsing a sequence interpreted as [std::begin(sequence), std::end(sequence))
     * using basic_string_view for sequence results.
     * The result is a std::pair with the position of the first unparsed token as first
     * element and the result of the parse as the second.
     */
    template <typename SequenceType>
    constexpr auto parse(const SequenceType &sequence) const {
        return parse(std::begin(sequence), std::end(sequence));
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
