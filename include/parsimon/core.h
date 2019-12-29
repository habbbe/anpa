#ifndef PARSIMON_CORE_H
#define PARSIMON_CORE_H

#include <functional>
#include <string_view>
#include "parsimon/result.h"
#include "parsimon/state.h"
#include "parsimon/settings.h"
#include "parsimon/types.h"

namespace parsimon {

template <typename P>
struct parser;

/**
 * Monadic bind for the parser
 */
template <typename P, typename F>
inline constexpr auto operator>>=(parser<P> p, F f) {
    return parser([=](auto& s) {
        if (auto&& result = p(s)) {
            return f(*std::forward<decltype(result)>(result))(s);
        } else {
            using new_return_type = std::decay_t<decltype(*f(*std::forward<decltype(result)>(result))(s))>;
            return s.template return_fail_change_result<new_return_type>(result);
        }
    });
}

/**
 * Lifts a type to the parser monad by forwarding the provided arguments to its constructor.
 */
template <typename T, typename... Args>
constexpr auto mreturn_emplace(Args&&... args) {
    return parser([args = std::make_tuple(std::forward<Args>(args)...)](auto& s) mutable {
        return std::apply([&s](auto&&... args){
            return s.template return_success_emplace<T>(std::forward<Args>(args)...);
        }, std::move(args));
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
        return P(p)(s);
    }

    template <typename InternalState>
    constexpr auto parse_internal(InternalState&& state) const {
        return std::pair(std::forward<InternalState>(state), P(p)(state));
    }

    /**
     * Begin parsing a sequence interpreted as [begin, end) with state,
     * using the supplied conversion function when returning sequence results.
     * The result is a std::pair with the parser state as the first
     * element and the result of the parse as the second.
     */
    template <typename Settings = default_parser_settings, typename Iterator, typename State>
    constexpr auto parse_with_state(Iterator begin,
                                    Iterator end,
                                    State&& user_state) const {
        return parse_internal(parser_state(begin, end, std::forward<State>(user_state), Settings()));
    }

    /**
     * Begin parsing a sequence interpreted as [std::begin(sequence), std::end(sequence)) with state,
     * using basic_string_view for sequence results.
     * The result is a std::pair with the parser state as the first
     * element and the result of the parse as the second.
     */
    template <typename Settings = default_parser_settings, typename SequenceType, typename State>
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
    template <typename Settings = default_parser_settings, typename ItemType, size_t N, typename State>
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
    template <typename Settings = default_parser_settings, typename Iterator>
    constexpr auto parse(Iterator begin, Iterator end) const {
        return parse_internal(parser_state_simple(begin, end, Settings()));
    }

    /**
     * Begin parsing a sequence interpreted as [std::begin(sequence), std::end(sequence))
     * using basic_string_view for sequence results.
     * The result is a std::pair with the parser state as the first
     * element and the result of the parse as the second.
     */
    template <typename Settings = default_parser_settings, typename SequenceType>
    constexpr auto parse(const SequenceType& sequence) const {
        return parse<Settings>(std::begin(sequence), std::end(sequence));
    }

    /**
     * Begin parsing a null terminated string literal
     * using basic_string_view for sequence results.
     * The result is a std::pair with the parser state as the first
     * element and the result of the parse as the second.
     */
    template <typename Settings = default_parser_settings, typename ItemType, size_t N>
    constexpr auto parse(const ItemType (&sequence)[N]) const {
        return parse<Settings>(sequence, sequence + N - 1);
    }

    /**
     * Make this parser a parser that assigns its result to the provided output iterator
     * upon success, as well as returning `none` as the result of the parse.
     * Note that any pointer type is also an output iterator.
     */
    template <typename It,
              typename = std::enable_if_t<types::iterator_is_category_v<It, std::output_iterator_tag>>>
    constexpr auto operator[](It it) const {
        return *this >>= [=](auto&& r) {
            *it = std::forward<decltype(r)>(r);
            return mreturn_emplace<none>();
        };
    }
};

}

#endif // PARSIMON_CORE_H
