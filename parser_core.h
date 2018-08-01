#ifndef PARSER_CORE_H
#define PARSER_CORE_H

#include <variant>
#include <string_view>
#include <functional>
#include <type_traits>

#define return_fail_default(s) return_fail<typename std::decay_t<decltype(s)>::string_result_type>();
#define return_fail_default_error(s, e) return_fail<typename std::decay_t<decltype(s)>::string_result_type>(e);

namespace parse {

template <typename Iterator>
constexpr bool is_random_access_iterator() {
    using category = typename std::iterator_traits<std::decay_t<Iterator>>::iterator_category;
    return std::is_same_v<category, std::random_access_iterator_tag>;
}

/**
 * Check if the result is a successful one
 */
//template <typename Result>
//static constexpr bool has_result(const Result& r) {
//    return r.operator bool();
//}

/**
 * Unpack the result to the underlying successful result.
 * Note: Has undefined behavior if the result is not successful. Check has_result() before.
 */
//template <typename Result>
//static constexpr decltype(auto) get_result(const Result& r) {
//    return *r;
//}

/**
 * Unpack the result to the underlying error.
 * Note: Has undefined behavior if the result is successful. Check !has_result() before.
 */
//template <typename Result>
//static constexpr auto get_error(const Result& r) {
//    return std::get<0>(r);
//}

template <typename P>
struct parser;

using default_error_type = const char*;
constexpr auto error_handling = true;

template <typename T>
using variant = std::variant<default_error_type, T>;

template <typename R, typename ErrorType>
struct result {

    constexpr static auto has_error_handling = !std::is_void_v<ErrorType>;

    typename std::conditional<has_error_handling, variant<R>, std::optional<R>>::type res;

    template <size_t N, typename... V>
    result(std::in_place_index_t<N> p, V&&...v) : res{p, std::forward<V>(v)...} {}

    template <typename... V>
    result(std::in_place_t t, V&&...v) : res{std::optional<R>(t, std::forward<V>(v)...)} {}

    template <typename... V>
    result(std::optional<R> o) : res{o} {}

    auto const& operator*() {
        if constexpr (has_error_handling) {
            return std::get<1>(res);
        } else {
            return *res;
        }
    }

    operator bool() {
        if constexpr (has_error_handling) {
            return res.index() == 1;
        } else {
            return res.operator bool();
        }
    }

    auto const& error() {
        static_assert(has_error_handling, "No error handling");
        return std::get<0>(res);
    }
};
//template <template<typename> typename Container, typename... Rs>
//parse_result(Container<Rs...> &&c) ->
//    parse_result<std::conditional_t<sizeof...(Rs) == 1, void, std::decay_t<decltype(std::get<0>(c))>>,
//                 std::conditional_t<sizeof...(Rs) == 1, std::decay_t<decltype(*c)>, std::decay_t<decltype(std::get<1>(c))>>>;

// Convenience function for returning a succesful parse.
template <typename Res>
static constexpr auto return_success(Res&& res) {
    if constexpr (error_handling) {
        return result<std::decay_t<Res>, default_error_type>(std::in_place_index_t<1>(), std::forward<Res>(res));
    } else {
        return result<std::decay_t<Res>, void>(std::in_place_t(), std::forward<Res>(res));
    }
}

// Convenience function for returning a succesful parse.
template <typename T, typename... Res>
static constexpr auto return_success_forward(Res&&... res) {
    if constexpr (error_handling) {
        return result<T, default_error_type>(std::in_place_index_t<1>(), std::forward<Res>(res)...);
    } else {
        return result<T, void>(std::in_place_t(), std::forward<Res>(res)...);
    }
}

// Convenience function for returning a failed parse with state and type of result.
template <typename Res, typename Error>
static constexpr auto return_fail(Error &&error) {
    if constexpr (error_handling) {
        return result<Res, default_error_type>(std::in_place_index_t<0>(), std::forward<Error>(error));
    } else {
        return result<Res, void>(std::optional<Res>());
    }
}

template <typename Res>
static constexpr auto return_fail() {
    return return_fail<Res>("Parsing error");
}

/**
 * Class for the parser state.
 */
template <typename Iterator, typename StringResultConversion>
struct parser_state_simple {
    Iterator position;
    const Iterator end;
    StringResultConversion conversion_function;

    using string_result_type = decltype(conversion_function(std::declval<Iterator>(), std::declval<Iterator>()));

    constexpr parser_state_simple(Iterator begin, Iterator end, StringResultConversion convert) :
        position{begin}, end{end}, conversion_function{convert} {}

    constexpr auto front() const {return *position;}

    // If we have a random access iterator, just use std::distance, otherwise
    // iterate so that we don't have to go all the way to end
    constexpr auto has_at_least(long n) {
        if constexpr (is_random_access_iterator<Iterator>()) {
           return std::distance(position, end) >= n;
        } else {
            auto start = position;
            for (long i = 0; i<n; ++i, ++start)
                if (start == end) return false;
            return true;
        }
    }

    constexpr auto empty() const {return position == end;}
    constexpr auto convert(Iterator begin, Iterator end) const {return conversion_function(begin, end);}
    constexpr auto convert(Iterator begin, size_t size) const {return convert(begin, begin+size);}
    constexpr auto convert(Iterator end) const {return convert(position, end);}
    constexpr auto convert(size_t size) const {return convert(position+size);}
    constexpr auto set_position(Iterator p) {position = p;}
    constexpr auto advance(size_t n) {std::advance(position, n);}
private:
    parser_state_simple(const parser_state_simple &other) = delete;
};

constexpr auto string_view_convert = [](auto begin, auto end) {
    using type = std::decay_t<decltype(*begin)>;
    if constexpr (std::is_pointer_v<decltype(begin)>)
        return std::basic_string_view<type>(begin, std::distance(begin, end));
    else
        return std::basic_string_view<type>(&*begin, std::distance(begin, end));
};

/**
 * Class for the parser state. Contains the string to be parsed, along
 * with the user provided state
 */
template <typename Iterator, typename StringResultConversion, typename UserState>
struct parser_state: public parser_state_simple<Iterator, StringResultConversion> {
    UserState& user_state;

    constexpr parser_state(Iterator begin, Iterator end, UserState& state, StringResultConversion convert)
        : parser_state_simple<Iterator, StringResultConversion>{begin, end, convert}, user_state{state} {}
};

//template <typename T, typename Iterator, typename StringConversionFunction, typename S = void>
//using type = std::function<variant<T>(std::conditional_t<std::is_void<S>::value, parser_state_simple<Iterator, StringConversionFunction> &, parser_state<Iterator, StringConversionFunction, S> &>)>;

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

/**
 * Monadic bind for the parser
 */
template <typename Parser, typename F>
static constexpr auto operator>>=(Parser p, F f) {
    return parser([=](auto &s) {
        if (auto result = apply(p, s); result) {
            return f(*result)(s);
        } else {
            using new_return_type = std::decay_t<decltype(*(f(*result)(s)))>;
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
        return return_success_forward<T>(args...);
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
    return parser([t = std::forward<T>(t)](auto &) {
        return return_success(t);
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
        parser_state state(begin, end, user_state, convert);
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
        auto state = parser_state_simple(begin, end, convert);
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
