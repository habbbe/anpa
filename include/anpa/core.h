#ifndef PARSIMON_CORE_H
#define PARSIMON_CORE_H

#include <functional>
#include "anpa/result.h"
#include "anpa/state.h"
#include "anpa/settings.h"
#include "anpa/types.h"

namespace anpa {


// Apply a parser to a state and return the result.
// This application unwraps arbitrary layers of callables so that one can
// wrap the parser to enable recursion.
template <typename Parser, typename S>
constexpr auto apply(Parser p, S& s) {
    if constexpr (std::is_invocable_v<Parser>) {
        return apply(p(), s);
    } else {
        return p(s);
    }
}

template <typename P>
struct parser;

/**
 * Monadic bind for the parser
 *
 * If the parse succeeds, then `f` will be called with the result of
 * the parse.
 *
 * @param p the parser to bind
 * @param f a functor with the following signature:
 *            NewMonad(auto&& parse_result)
 */
template <typename P, typename Fn>
inline constexpr auto operator>>=(parser<P> p, Fn f) {
    return parser([=](auto& s) {
        if (auto&& result = apply(p, s)) {
            return apply(f(*std::forward<decltype(result)>(result)), s);
        } else {
            using new_return_type = decltype(*apply(f(*std::forward<decltype(result)>(result)), s));
            return s.template return_fail_change_result<new_return_type>(result);
        }
    });
}

/**
 * Lift a value to the parser monad
 *
 * Do not use this function to lift non-copyable types, as this will make
 * the whole parser non-copyable, and things will break.
 *
 * If you want to lift a non-copyable value, instead use anpa::lift
 * Example:
 * @code
 *
 * auto p1 = mreturn(std::make_unique<int>(4)); // Wrong
 *
 * auto p2 = lift([]() {return std::make_unique<int>(4);}); // OK
 *
 * auto p3 = integer() >>= [](int i) {
 *     mreturn(std::make_unique<int>(i));
 * }; // Wrong
 *
 * auto p4 = lift([](int i) {
 *     return std::make_unique<int>(i);
 * }, integer()); // OK
 *
 * @endcode
 */
template <typename T>
constexpr auto mreturn(T&& t) {
    types::assert_copyable_mreturn<T>();
    return parser([t = std::forward<T>(t)](auto& s) {
        return s.return_success(t);
    });
}

/**
 * Lifts a type to the parser monad by forwarding the provided arguments to its constructor.
 *
 * Do not use this function if any of the arguments are non-copyable types, as this will make
 * the whole parser non-copyable, and things will break.
 *
 * If any of the arguments are non-copyable value, instead use anpa::lift
 * @see anpa::mreturn for an example
 *
 * Or anpa::lift_value, if the value should be constructed in-place with the parser
 * results forwarded to its constructor.
 */
template <typename T, typename... Args>
constexpr auto mreturn_emplace(Args&&... args) {
    (types::assert_copyable_mreturn<Args>(), ...);
    return parser([args = std::make_tuple(std::forward<Args>(args)...)](auto& s) {
        return std::apply([&s](auto... args){
            return s.template return_success_emplace<T>(std::move(args)...);
        }, args);
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
     * Begin parsing a sequence described by `[begin, end)` with state
     *
     * The result is a std::pair with the anpa::parser_state as the first
     * element and the result of the parse as the second.
     *
     * @tparam the parser settings to use (default: `default_parser_settings`)
     */
    template <typename Settings = default_parser_settings, typename InputIt, typename State>
    constexpr auto parse_with_state(InputIt begin,
                                    InputIt end,
                                    State&& user_state) const {
        return parse_internal(parser_state(begin, end, std::forward<State>(user_state), Settings()));
    }

    /**
     * Begin parsing a sequence described by `[std::begin(sequence), std::end(sequence))` with state
     *
     * The result is a std::pair with the anpa::parser_state as the first
     * element and the result of the parse as the second.
     *
     * @tparam the parser settings to use (default: `default_parser_settings`)
     */
    template <typename Settings = default_parser_settings, typename SequenceType, typename State>
    constexpr auto parse_with_state(const SequenceType& sequence,
                                    State&& user_state) const {
        return parse_with_state<Settings>(std::begin(sequence),
                                std::end(sequence),
                                std::forward<State>(user_state));
    }

    /**
     * Begin parsing a null terminated string literal with state
     *
     * The result is a std::pair with the anpa::parser_state as the first
     * element and the result of the parse as the second.
     *
     * @tparam the parser settings to use (default: `default_parser_settings`)
     */
    template <typename Settings = default_parser_settings, typename ItemType, size_t N, typename State>
    constexpr auto parse_with_state(const ItemType (&sequence)[N],
                                    State&& user_state) const {
        return parse_with_state<Settings>(sequence,
                                sequence + N - 1,
                                std::forward<State>(user_state));
    }

    /**
     * Begin parsing the sequence described by [begin, end)
     *
     * The result is a std::pair with anpa::parser_state_simple as the first
     * element and the result of the parse as the second.
     *
     * @tparam the parser settings to use (default: `default_parser_settings`)
     */
    template <typename Settings = default_parser_settings, typename InputIt>
    constexpr auto parse(InputIt begin, InputIt end) const {
        return parse_internal(parser_state_simple(begin, end, Settings()));
    }

    /**
     * Begin parsing a sequence described by `[std::begin(sequence), std::end(sequence))`
     *
     * The result is a std::pair with anpa::parser_state_simple as the first
     * element and the result of the parse as the second.
     *
     * @tparam the parser settings to use (default: `default_parser_settings`)
     */
    template <typename Settings = default_parser_settings, typename SequenceType>
    constexpr auto parse(const SequenceType& sequence) const {
        return parse<Settings>(std::begin(sequence), std::end(sequence));
    }

    /**
     * Begin parsing a null terminated string literal
     *
     * The result is a std::pair with anpa::parser_state_simple as the first
     * element and the result of the parse as the second.
     *
     * @tparam the parser settings to use (default: `default_parser_settings`)
     */
    template <typename Settings = default_parser_settings, typename ItemType, size_t N>
    constexpr auto parse(const ItemType (&sequence)[N]) const {
        return parse<Settings>(sequence, sequence + N - 1);
    }

    /**
     * Make this parser a parser that assigns its result to the provided output iterator
     * upon success, as well as returning `empty_result` as the result of the parse.
     * Note that any pointer type is also an output iterator.
     */
    template <typename OutputIt,
              typename = std::enable_if_t<types::iterator_is_category_v<OutputIt, std::output_iterator_tag>>>
    constexpr auto operator[](OutputIt it) const {
        return *this >>= [it](auto&& s) {
            *it = std::forward<decltype(s)>(s);
            return mreturn_emplace<empty_result>();
        };
    }
};

}

#endif // PARSIMON_CORE_H
