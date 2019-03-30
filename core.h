#ifndef PARSER_CORE_H
#define PARSER_CORE_H

#include <functional>
#include <string_view>
#include "result.h"
#include "state.h"
#include "settings.h"
#include "types.h"

namespace parse {

template <typename Result, typename ErrorType, typename State, typename Iterator, typename StringConversionFunction = std::remove_const_t<decltype(string_view_convert)>, typename Settings = parser_settings>
using type = std::function<result<Result, ErrorType>(std::conditional_t<std::is_void<State>::value, parser_state_simple<Iterator, StringConversionFunction, Settings>, parser_state<Iterator, StringConversionFunction, State, Settings>>&)>;

/**
 * Apply a parser to a state and return the result.
 * This application unwraps arbitrary layers of callables so that one can
 * wrap the parser to enable recursion.
 */
template <typename P, typename S>
constexpr auto apply(P p, S& s) {
    if constexpr (std::is_invocable_v<P>) {
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
inline constexpr auto operator>>=(Parser p, F f) {
    return parser([=](auto& s) {
        if (const auto& result = apply(p, s)) {
            return apply(f(std::move(*result)), s);
        } else {
            using new_return_type = std::decay_t<decltype(*apply(f(std::move(*result)), s))>;
            return s.template return_fail_change_result<new_return_type>(result);
        }
    });
}

/**
 * Lifts a type to the parser monad by forwarding the provided arguments to its constructor.
 */
template <typename T, typename... Args>
constexpr auto mreturn_emplace(Args&&... args) {
    return parser([=](auto& s) {
        return s.template return_success_emplace<T>(args...);
    });
}

/**
 * Lift a value to the parser monad
 */
template <typename T>
constexpr auto mreturn(T&& t) {
    return mreturn_emplace<T>(std::forward<T>(t));
}

/**
 * Lift a value to the parser monad. Templated general version. The returned value must be in global scope.
 */
template <auto& T>
constexpr auto mreturn() { return parser([](auto& s) { return s.return_success(T); }); }

/**
 * Lift a value to the parser monad. Templated general version. Use this for literal types you know at
 * compile time.
 */
template <auto T>
constexpr auto mreturn() { return parser([](auto& s) { return s.return_success(T); }); }

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
    constexpr auto operator()(State& s) const {
        return apply(p, s);
    }

    template <typename InternalState>
    constexpr auto parse_internal(InternalState&& state) const {
        return std::pair(std::forward<InternalState>(state), apply(p, state));
    }

    /**
     * Begin parsing a sequence interpreted as [begin, end) with state,
     * using the supplied conversion function when returning sequence results.
     * The result is a std::pair with the parser state as the first
     * element and the result of the parse as the second.
     */
    template <typename Settings = parser_settings, typename Iterator, typename State, typename ConversionFunction>
    constexpr auto parse_with_state(Iterator begin,
                                    Iterator end,
                                    State&& user_state,
                                    ConversionFunction convert) const {
        return parse_internal(parser_state(begin, end, std::forward<State>(user_state), convert, Settings()));
    }

    /**
     * Begin parsing a sequence interpreted as [begin, end) with state,
     * using basic_string_view for sequence results.
     * The result is a std::pair with the parser state as the first
     * element and the result of the parse as the second.
     */
    template <typename Settings = parser_settings, typename Iterator, typename State>
    constexpr auto parse_with_state(Iterator begin,
                                    Iterator end,
                                    State&& user_state) const {
        return parse_with_state<Settings>(begin,
                                end,
                                std::forward<State>(user_state),
                                string_view_convert);
    }

    /**
     * Begin parsing a sequence interpreted as [std::begin(sequence), std::end(sequence)) with state,
     * using basic_string_view for sequence results.
     * The result is a std::pair with the parser state as the first
     * element and the result of the parse as the second.
     */
    template <typename Settings = parser_settings, typename SequenceType, typename State>
    constexpr auto parse_with_state(const SequenceType& sequence,
                                    State&& user_state) const {
        return parse_with_state<Settings>(std::begin(sequence),
                                std::end(sequence),
                                std::forward<State>(user_state));
    }

    /**
     * Begin parsing a null terminated string literal,
     * using basic_string_view for sequence results.
     * The result is a std::pair with the parser state as the first
     * element and the result of the parse as the second.
     */
    template <typename Settings = parser_settings, typename ItemType, size_t N, typename State>
    constexpr auto parse_with_state(const ItemType (&sequence)[N],
                                    State&& user_state) const {
        return parse_with_state<Settings>(sequence,
                                sequence + N - 1,
                                std::forward<State>(user_state));
    }

    /**
     * Begin parsing the sequence described by [begin, end),
     * using the supplied conversion function when returning sequence results.
     * The result is a std::pair with the parser state as the first
     * element and the result of the parse as the second.
     */
    template <typename Settings = parser_settings, typename Iterator, typename ConversionFunction>
    constexpr auto parse(Iterator begin, Iterator end, ConversionFunction convert) const {
        return parse_internal(parser_state_simple(begin, end, convert, Settings()));
    }

    /**
     * Begin parsing the sequence described by [begin, end),
     * using basic_string_view for sequence results.
     * The result is a std::pair with the parser state as the first
     * element and the result of the parse as the second.
     */
    template <typename Settings = parser_settings, typename Iterator>
    constexpr auto parse(Iterator begin, Iterator end) const {
        return parse<Settings>(begin, end, string_view_convert);
    }

    /**
     * Begin parsing a sequence interpreted as [std::begin(sequence), std::end(sequence))
     * using basic_string_view for sequence results.
     * The result is a std::pair with the parser state as the first
     * element and the result of the parse as the second.
     */
    template <typename Settings = parser_settings, typename SequenceType>
    constexpr auto parse(const SequenceType& sequence) const {
        return parse<Settings>(std::begin(sequence), std::end(sequence));
    }

    /**
     * Begin parsing a null terminated string literal
     * using basic_string_view for sequence results.
     * The result is a std::pair with the parser state as the first
     * element and the result of the parse as the second.
     */
    template <typename Settings = parser_settings, typename ItemType, size_t N>
    constexpr auto parse(const ItemType (&sequence)[N]) const {
        return parse<Settings>(sequence, sequence + N - 1);
    }

    /**
     * Class member of mreturn_emplace. For general monad use.
     */
    template <typename T, typename... Args>
    static inline constexpr auto mreturn_emplace(Args&&... args) {
        return parse::mreturn_emplace<T>(std::forward<Args>(args)...);
    }

    /**
     * Class member of mreturn. For general monad use.
     */
    template <typename T>
    static inline constexpr auto mreturn(T&& v) {
        return parse::mreturn(std::forward<T>(v));
    }

    /**
     * Make this parser a parser that assigns its result to the provided output iterator
     * upon success, as well as returning `true` as the result of the parse.
     * Note that any pointer type is also an output iterator.
     */
    template <typename T,
              typename = std::enable_if_t<parse::types::iterator_is_category_v<T, std::output_iterator_tag>>>
    constexpr auto operator[](T t) const {
        return *this >>= [=](auto&& s) {
            *t = std::forward<decltype(s)>(s);
            return mreturn_emplace<bool>(true);
        };
    }
};

}

#endif // PARSER_CORE_H
