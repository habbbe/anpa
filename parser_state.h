#ifndef PARSER_STATE_H
#define PARSER_STATE_H

#include <iterator>
#include "parser_result.h"
#include "parse_algorithm.h"

namespace parse {

/**
 * Class for the parser state.
 */
template <typename Iterator, typename StringResultConversion, typename Settings>
struct parser_state_simple {
    Iterator position;
    const Iterator end;
    const StringResultConversion conversion_function;

    using settings = Settings;
    constexpr static bool error_handling = Settings::error_handling;


    using default_result_type = decltype(conversion_function(std::declval<Iterator>(), std::declval<Iterator>()));

    constexpr parser_state_simple(Iterator begin, Iterator end, StringResultConversion convert, Settings) :
        position{begin}, end{end}, conversion_function{convert} {}

    template<typename State>
    constexpr parser_state_simple(Iterator begin, Iterator end, const State &other) :
        parser_state_simple(begin, end, other.conversion_function, typename State::settings()){}

    constexpr auto front() const {return *position;}

    constexpr auto has_at_least(long n) {
        return algorithm::contains_elements(position, end, n);
    }

    constexpr auto empty() const {return position == end;}
    constexpr auto convert(Iterator begin, Iterator end) const {return conversion_function(begin, end);}
    constexpr auto convert(Iterator begin, size_t size) const {return convert(begin, std::next(begin, size));}
    constexpr auto convert(Iterator end) const {return convert(position, end);}
    constexpr auto convert(size_t size) const {return convert(std::next(position, size));}
    constexpr auto set_position(Iterator p) {position = p;}
    constexpr auto advance(size_t n) {std::advance(position, n);}

    // Convenience function for returning a succesful parse.
    template <typename T, typename... Args>
    constexpr auto return_success_forward(Args&&... args) {
        if constexpr (error_handling) {
            return result<std::decay_t<T>, default_error_type>(std::in_place_index<1>, std::forward<Args>(args)...);
        } else {
            return result<std::decay_t<T>, void>(std::in_place, std::forward<Args>(args)...);
        }
    }

    // Convenience function for returning a succesful parse.
    template <typename Res>
    constexpr auto return_success(Res&& res) {
        return return_success_forward<Res>(std::forward<Res>(res));
    }

    // Convenience function for returning a failed parse with state and type of result.
    template <typename Res, typename Error>
    constexpr auto return_fail(Error &&error) {
        if constexpr (error_handling) {
            return result<Res, std::decay_t<decltype(error)>>(std::in_place_index<0>, std::forward<Error>(error));
        } else {
            return result<Res, void>();
        }
    }

    template <typename Error>
    constexpr auto return_fail(Error &&error) { return return_fail<default_result_type>(std::forward<Error>(error)); }

    template <typename Res>
    constexpr auto return_fail() { return return_fail<Res>("Parsing error"); }

    constexpr auto return_fail() { return return_fail<default_result_type>(); }

    template <typename Res, typename Error>
    constexpr auto return_fail(const result<Res, Error> &res) {
        if constexpr (error_handling) {
            return result<Res, Error>(std::in_place_index<0>, std::forward<Error>(res.error));
        } else {
            return result<Res, void>();
        }

    }



private:
    parser_state_simple(const parser_state_simple &other) = delete;
};

constexpr auto string_view_convert = [](auto begin, auto end) {
    using type = std::decay_t<decltype(*begin)>;
    auto distance = std::distance(begin, end);
    if constexpr (std::is_pointer_v<decltype(begin)>)
        return std::basic_string_view<type>(begin, distance);
    else {
        return std::basic_string_view<type>(begin.operator->(), distance);
    }
};

/**
 * Class for the parser state. Contains the user provided state
 */
template <typename Iterator, typename StringResultConversion, typename Settings, typename UserState>
struct parser_state: public parser_state_simple<Iterator, StringResultConversion, Settings> {
    UserState& user_state;

    constexpr parser_state(Iterator begin, Iterator end, UserState& state, StringResultConversion convert, Settings settings)
        : parser_state_simple<Iterator, StringResultConversion, Settings>{begin, end, convert, settings}, user_state{state} {}

    template<typename State>
    constexpr parser_state(Iterator begin, Iterator end, const State &other) :
        parser_state(begin, end, other.user_state, other.conversion_function, typename State::settings()){}
};


}

#endif // PARSER_STATE_H
