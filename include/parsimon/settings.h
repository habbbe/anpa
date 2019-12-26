#ifndef PARSIMON_SETTINGS_H
#define PARSIMON_SETTINGS_H

#include <type_traits>
#include <iterator>

namespace parsimon {

constexpr auto default_convert = [](auto begin, auto end) {
    return std::make_pair(begin, end);
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

template <bool ErrorHandling = false, auto& Convert = string_view_convert>
struct parser_settings {
    constexpr static bool error_handling = ErrorHandling;
    constexpr static auto conversion_function = Convert;
};

using default_parser_settings = parser_settings<false, string_view_convert>;

}

#endif // PARSIMON_SETTINGS_H
