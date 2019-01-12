#ifndef PARSER_STATE_H
#define PARSER_STATE_H

#include <iterator>
#include "result.h"
#include "algorithm.h"

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


    constexpr auto has_at_least(long n) {
        return algorithm::contains_elements(position, end, n);
    }

    constexpr bool empty() const {return position == end;}
    constexpr decltype(auto) get_at(Iterator it) const {return *it;}
    constexpr decltype(auto) front() const {return get_at(position);}
    constexpr decltype(auto) convert(Iterator begin, Iterator end) const {return conversion_function(begin, end);}
    constexpr decltype(auto) convert(Iterator begin, size_t size) const {return convert(begin, std::next(begin, size));}
    constexpr decltype(auto) convert(Iterator end) const {return convert(position, end);}
    constexpr decltype(auto) convert(size_t size) const {return convert(std::next(position, size));}
    constexpr void set_position(Iterator p) {position = p;}
    constexpr void advance(size_t n) {std::advance(position, n);}

    // Convenience function for returning a succesful parse.
    template <typename T, typename... Args>
    constexpr auto return_success_forward(Args&&... args) {
        using Res = std::decay_t<T>;
        if constexpr (error_handling) {
            return result<std::decay_t<Res>, default_error_type>(std::in_place_index<1>, std::forward<Args>(args)...);
        } else {
            return result<std::decay_t<Res>, void>(std::in_place, std::forward<Args>(args)...);
        }
    }

    // Convenience function for returning a succesful parse.
    template <typename Res>
    constexpr auto return_success(Res&& res) {
        return return_success_forward<std::decay_t<Res>>(std::forward<Res>(res));
    }

    // Convenience function for returning a failed parse with state and type of result.
    template <typename Res, typename Error>
    constexpr auto return_fail_error(Error &&error) {
        using T = std::decay_t<Res>;
        if constexpr (error_handling) {
            return result<T, std::decay_t<decltype(error)>>(std::in_place_index<0>, std::forward<Error>(error));
        } else {
            return result<T, void>();
        }
    }

    template <typename Error>
    constexpr auto return_fail_error_default(Error &&error) { return return_fail_error<default_result_type>(std::forward<Error>(error)); }

    template <typename Res, typename Res2, typename Error>
    constexpr auto return_fail_change_result(const result<Res2, Error> &res) {
        using T = std::decay_t<Res>;
        if constexpr (error_handling) {
            return result<T, Error>(std::in_place_index<0>, res.error());
        } else {
            return result<T, void>();
        }
    }

    template <typename Res, typename Error>
    constexpr auto return_fail_result_default(const result<Res, Error> &res) {
        return return_fail_change_result<default_result_type>(res);
    }

    template <typename Res, typename Error>
    constexpr auto return_fail_result(const result<Res, Error> &res) {
        return return_fail_change_result<Res>(res);
    }

    template <typename Res>
    constexpr auto return_fail() { return return_fail_error<Res>("Parsing error"); }

    constexpr auto return_fail() { return return_fail<default_result_type>(); }
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
    UserState user_state;

    constexpr parser_state(Iterator begin, Iterator end, UserState &&state, StringResultConversion convert, Settings settings)
        : parser_state_simple<Iterator, StringResultConversion, Settings>{begin, end, convert, settings}, user_state{std::forward<UserState>(state)} {}

    template<typename State>
    constexpr parser_state(Iterator begin, Iterator end, const State &other) :
        parser_state(begin, end, other.user_state, other.conversion_function, typename State::settings()){}
};
template <typename Iterator, typename StringResultConversion, typename Settings, typename UserState>
parser_state(Iterator, Iterator, UserState&&, StringResultConversion, Settings) ->
parser_state<Iterator, StringResultConversion, Settings,
UserState>;

}

#endif // PARSER_STATE_H
