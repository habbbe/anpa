#ifndef PARSER_SETTINGS_H
#define PARSER_SETTINGS_H

#include <type_traits>
#include <iterator>

namespace parse {

constexpr auto string_view_convert = [](auto begin, auto end) {
    using type = std::decay_t<decltype(*begin)>;
    auto distance = std::distance(begin, end);
    if constexpr (std::is_pointer_v<decltype(begin)>)
        return std::basic_string_view<type>(begin, distance);
    else {
        return std::basic_string_view<type>(begin.operator->(), distance);
    }
};

struct parser_settings {
    constexpr static bool error_handling = false;
    constexpr static auto conversion_function = string_view_convert;
};

}

#endif // PARSER_SETTINGS_H
