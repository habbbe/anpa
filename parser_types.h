#ifndef PARSER_TYPES_H
#define PARSER_TYPES_H

#include <type_traits>
#include <iterator>

namespace parse::types {

template <typename Iterator, typename Tag>
constexpr bool iterator_is_category_v =
    std::is_same_v<typename std::iterator_traits<std::decay_t<Iterator>>::iterator_category, Tag>;
}

#endif // PARSER_TYPES_H
