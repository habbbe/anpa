#ifndef PARSIMON_SETTINGS_H
#define PARSIMON_SETTINGS_H

#include <type_traits>
#include <iterator>
#include <array>

namespace parsimon {

/// Conversion function that returns ranges as a `std::pair<BeginIterator, EndIterator>`
constexpr auto iterator_convert = [](auto&& begin, auto&& end) {
    return std::make_pair(
        std::forward<decltype(begin)>(begin),
        std::forward<decltype(end)>(end));
};

/// Conversion function that returns ranges `std::basic_string_view<element_type>`
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
 * Parser settings.
 *
 * @tparam ErrorMessages set if error messages are desired.
 * @tparam Convert the functor to be used to create results for parsed ranges.
 *         It should have the following signature:
 *           `ResultType(auto begin_iterator, auto end_iterator)`
 */
template <bool ErrorMessages = false, auto& Convert = string_view_convert>
struct parser_settings {
    constexpr static bool error_messages = ErrorMessages;
    constexpr static auto conversion_function = Convert;
};

/**
 * The default parser settings. No error messages, and range results are
 * returned as `std::basic_string_view<element_type>`
 */

using default_parser_settings = parser_settings<>;

}

#endif // PARSIMON_SETTINGS_H
