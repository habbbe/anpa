#ifndef PARSER_TYPES_H
#define PARSER_TYPES_H

#include <type_traits>
#include <iterator>

namespace parse {

/**
 * Empty type used to denote empty-ish results.
 * This type is also used to denote the lack of optional parameters.
 */
using none = std::tuple<>;

}

namespace parse::types {

template <typename T, typename ... Ts>
constexpr bool is_one_of = (std::is_same_v<T, Ts> || ...);

template <typename T>
constexpr bool is_string_literal_type = is_one_of<T, char, wchar_t, char16_t, char32_t>;

template <typename T>
using enable_if_string_literal_type = std::enable_if_t<is_string_literal_type<T>>;

template <typename Iterator, typename Tag>
constexpr bool iterator_is_category_v =
    std::is_same_v<typename std::iterator_traits<std::decay_t<Iterator>>::iterator_category, Tag>;
}



#endif // PARSER_TYPES_H
