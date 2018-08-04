#ifndef PARSER_STATE_H
#define PARSER_STATE_H

#include <iterator>
#include "parser_result.h"

namespace parse {

template <typename Iterator>
constexpr bool is_random_access_iterator() {
    using category = typename std::iterator_traits<std::decay_t<Iterator>>::iterator_category;
    return std::is_same_v<category, std::random_access_iterator_tag>;
}

/**
 * Class for the parser state.
 */
template <typename Iterator, typename StringResultConversion, typename Settings>
struct parser_state_simple {
    Iterator position;
    const Iterator end;
    StringResultConversion conversion_function;

    constexpr static bool error_handling = Settings::error_handling;

    using default_result_type = decltype(conversion_function(std::declval<Iterator>(), std::declval<Iterator>()));

    constexpr parser_state_simple(Iterator begin, Iterator end, StringResultConversion convert, Settings) :
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

    // Convenience function for returning a succesful parse.
    template <typename T, typename... Res>
    constexpr auto return_success_forward(Res&&... res) {
        if constexpr (error_handling) {
            return result<std::decay_t<T>, default_error_type>(std::in_place_index<1>, std::forward<Res>(res)...);
        } else {
            return result<std::decay_t<T>, void>(std::in_place, std::forward<Res>(res)...);
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
//template <typename Settings, typename Iterator, typename Fn>
//parser_state_simple(Settings, Iterator begin, Iterator end, Fn f) -> parser_state_simple<Settings, Iterator, Fn>;

/**
 * Class for the parser state. Contains the user provided state
 */
template <typename Iterator, typename StringResultConversion, typename Settings, typename UserState>
struct parser_state: public parser_state_simple<Iterator, StringResultConversion, Settings> {
    UserState& user_state;

    constexpr parser_state(Iterator begin, Iterator end, UserState& state, StringResultConversion convert, Settings settings)
        : parser_state_simple<Iterator, StringResultConversion, Settings>{begin, end, convert, settings}, user_state{state} {}
};


}

#endif // PARSER_STATE_H
