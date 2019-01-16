#ifndef PARSER_TYPES_H
#define PARSER_TYPES_H

#include <type_traits>
#include <iterator>

namespace parse {

/**
 * Empty type used to denote empty-ish results.
 * This type is also used to denote the lack of optional parameters.
 */
struct none {};

}

namespace parse::types {

template <typename State, typename P1, typename P2, typename ... Ps>
constexpr bool same_result = [](){
    using R1 = decltype(*apply(std::declval<P1>(), std::declval<State>()));
    using R2 = decltype(*apply(std::declval<P2>(), std::declval<State>()));
    if (!std::is_same_v<R1, R2>) return false;
    else if constexpr (sizeof...(Ps) == 0) return true;
    else return same_result<P2, Ps...>;
}();

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
