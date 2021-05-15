#ifndef PARSIMON_RANGE_H
#define PARSIMON_RANGE_H

#include <iterator>
#include "anpa/internal/algorithm.h"

namespace anpa {

/**
 * A range described by a begin and end iterator.
 */
template <typename InputIt>
struct range {
private:
    InputIt begin_it;
    InputIt end_it;
public:
    using size_type = size_t;
    using item_type = std::decay_t<decltype(*begin_it)>;

    constexpr range(InputIt begin, InputIt end) : begin_it{begin}, end_it{end} {}

    template <typename ItemType, size_t N>
    constexpr range(const ItemType (&seq)[N]) : begin_it{seq}, end_it{seq + N-1} {}

    template <typename ItemType, typename = std::enable_if_t<std::is_pointer_v<ItemType>>>
    constexpr range(const ItemType* seq) : begin_it{seq},
        end_it{seq + std::char_traits<ItemType>::length(seq)} {}

    constexpr bool operator==(const range& other) const {
        return algorithm::equal(begin_it, end_it, other.begin_it, other.end_it);
    }

    constexpr bool operator!=(const range& other) const {
        return !operator==(other);
    }

    constexpr bool empty() const {return begin_it == end_it;}

    constexpr size_type length() const {
        return static_cast<size_type>(std::distance(begin_it, end_it));
    }

    constexpr InputIt begin() const { return begin_it;}
    constexpr InputIt end() const { return end_it;}

    constexpr operator std::basic_string_view<item_type>() const {
        if (empty())
            return {};

        return {&*begin_it, length()};
    }
    constexpr operator std::basic_string<item_type>() const {
        return static_cast<std::basic_string<item_type>>(*this);
    }
};

}

#endif // PARSIMON_RANGE_H
